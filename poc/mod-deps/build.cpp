#include "../../ecow.hpp"

struct token {
  enum t { empty, str, num, lit, chr, other } type;
  std::string text;

  constexpr explicit operator bool() noexcept { return type != empty; }
  constexpr bool operator==(const token &o) const noexcept {
    return type == o.type && text == o.text;
  }
};

class tokeniser {
  std::ifstream m_in;
  token m_peeked{};

  token next_string() {
    std::string res{};
    while (true) {
      switch (char c = m_in.get()) {
      case 0:
        throw std::runtime_error("Invalid compile deps");
      case '\\':
        m_in.get();
        break;
      case '"':
        return {token::str, res};
      default:
        res += c;
      }
    }
  }
  token next_number(char first) {
    std::string res{first};
    while (true) {
      char c = m_in.get();
      if (c >= '0' && c <= '9') {
        res += c;
      } else {
        m_in.unget();
        return {token::num, res};
      }
    }
  }
  token next_literal(char first) {
    std::string res{first};
    while (true) {
      char c = m_in.get();
      if (c >= 'a' && c <= 'z') {
        res += c;
      } else {
        m_in.unget();
        return {token::lit, res};
      }
    }
  }

  token read_token() {
    while (true) {
      switch (char c = m_in.get()) {
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        continue;
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':
        return {token::chr, std::string{c}};
      case 't':
      case 'f':
        return next_literal(c);
      case '"':
        return next_string();
      default:
        if (c >= '0' && c <= '9')
          return next_number(c);
        return {token::other, ""};
      }
    }
  }

public:
  tokeniser() {
    const auto dep_file =
        ecow::impl::current_target()->build_folder() + "compile_deps.json";
    m_in = std::ifstream{dep_file};
    if (!m_in)
      throw std::runtime_error("Failure opening compile deps");
  }

  token next() {
    if (m_peeked) {
      auto res = m_peeked;
      m_peeked = {};
      return res;
    }
    return read_token();
  }

  token peek() {
    if (!m_peeked) {
      m_peeked = read_token();
    }
    return m_peeked;
  }
};
class stream {
  tokeniser m_t{};

  [[nodiscard]] bool peek(char c) {
    return m_t.peek() == token{token::chr, std::string{c}};
  }
  auto next() { return m_t.next(); }

  void consume(token exp) {
    auto t = next();
    if (exp != t)
      throw std::runtime_error("Expecting " + exp.text + ", got " + t.text +
                               " around " + next().text);
  }
  void consume(char c) {
    auto t = next();
    if (t.type != token::chr || t.text[0] != c)
      throw std::runtime_error("Expecting " + std::string{c} + " got " +
                               t.text + " around " + next().text);
  }
  auto consume(token::t exp) {
    auto t = next();
    if (exp != t.type)
      throw std::runtime_error("Expecting something else, got " + t.text +
                               " around " + next().text);
    return t.text;
  }

  void do_array(auto &&fn) {
    consume('[');
    while (!peek(']')) {
      fn(*this);
      if (peek(',')) {
        consume(',');
      } else {
        break;
      }
    }
    consume(']');
  }
  void do_any() {
    if (peek('[')) {
      do_array([](auto &s) { s.do_any(); });
      return;
    }
    if (peek('{')) {
      do_object([](auto &) {});
      return;
    }
    next();
  }

public:
  void do_object(auto &&fn) {
    consume('{');

    fn(*this);

    while (!peek('}')) {
      consume(token::str);
      consume(':');
      do_any();
      if (peek(',')) {
        consume(',');
      } else {
        break;
      }
    }
    consume('}');
  }
  void find_array_attr(const std::string &name, auto &&fn) {
    auto k = consume(token::str);
    consume(':');

    if (name == k) {
      do_array(fn);
    } else {
      do_any();
    }

    if (peek(',')) {
      consume(',');
      find_array_attr(name, fn);
    }
  }
  [[nodiscard]] bool find_object_attr(const std::string &name, auto &&fn) {
    while (true) {
      auto k = consume(token::str);
      consume(':');

      if (name == k) {
        do_object(fn);
        if (peek(',')) {
          consume(',');
        }
        return true;
      }

      do_any();
      if (peek(',')) {
        consume(',');
      } else {
        return false;
      }
    }
  }
  std::string find_string_attr(const std::string &name) {
    while (true) {
      auto k = consume(token::str);
      consume(':');

      if (name == k) {
        auto v = consume(token::str);
        if (peek(',')) {
          consume(',');
        }
        return v;
      }

      do_any();
      if (peek(',')) {
        consume(',');
      } else {
        throw std::runtime_error("Could not find attribute " + name);
      }
    }
  }
};

class test : public ecow::unit {
public:
  using unit::unit;

  void build_self() const override {
    stream{}.do_object([](auto &s) {
      s.find_array_attr("rules", [](auto &s) {
        s.do_object([](auto &s) {
          auto pcm = s.find_string_attr("primary-output");
          std::cerr << "pcm: " << pcm << std::endl;
          s.find_array_attr("requires", [&](auto &s) {
            s.do_object([&](auto &s) {
              auto dep = s.find_string_attr("source-path");
              std::cerr << pcm << " -> " << dep << std::endl;
            });
          });
        });
      });
    });

    throw "OK";
  }
  void create_self_cdb(std::ostream &o) const override {}
};

int main(int argc, char **argv) {
  using namespace ecow;

  auto all = unit::create<seq>("all");
  all->add_unit<test>("");

  all->add_unit<mod>("c");
  all->add_unit<mod>("b");

  auto a = all->add_unit<mod>("a");
  a->add_part("z");
  a->add_part("y");
  a->add_part("x");

  return run_main(all, argc, argv);
}
