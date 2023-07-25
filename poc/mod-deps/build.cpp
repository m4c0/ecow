#include "../../ecow.hpp"

class tokeniser {
  std::ifstream m_in;
  std::string m_peeked;

  std::string next_string() {
    std::string res{};
    while (true) {
      switch (char c = m_in.get()) {
      case 0:
        throw std::runtime_error("Invalid compile deps");
      case '\\':
        m_in.get();
        break;
      case '"':
        return res;
      default:
        res += c;
      }
    }
  }
  std::string next_number(char first) {
    std::string res{first};
    while (true) {
      char c = m_in.get();
      if (c >= '0' && c <= '9') {
        res += c;
      } else {
        m_in.unget();
        return res;
      }
    }
  }
  std::string next_literal(char first) {
    std::string res{first};
    while (true) {
      char c = m_in.get();
      if (c >= 'a' && c <= 'z') {
        res += c;
      } else {
        m_in.unget();
        return res;
      }
    }
  }

  std::string read_token() {
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
        return std::string{c};
      case 't':
      case 'f':
        return next_literal(c);
      case '"':
        return next_string();
      default:
        if (c >= '0' && c <= '9')
          return next_number(c);
        return "";
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

  std::string next() {
    if (m_peeked != "") {
      auto res = m_peeked;
      m_peeked = "";
      return res;
    }
    return read_token();
  }

  std::string peek() {
    if (m_peeked == "") {
      m_peeked = read_token();
    }
    return m_peeked;
  }
};

class test : public ecow::unit {
public:
  using unit::unit;

  void build_self() const override {
    tokeniser t{};

    // find_object();
    // find_array_attr("rules", []{
    //   find_object()
    //   auto pcm = find_string_attr("primary-output")
    //   find_object_attr("requires", []{
    //     auto dep = find_string_attr("source-path")

    std::string token;
    while ((token = t.next()) != "") {
      std::cerr << "T: " << token << std::endl;
    }

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
