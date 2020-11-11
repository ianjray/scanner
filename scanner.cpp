#include "scanner.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>


class Scanner::Impl
{
public:
    explicit Impl(Scanner *parent) : m_buffer{}, m_lexeme{}, m_next{}, m_state{}, m_backstep{}, m_chain{parent}
    {
    }

    void assign(const std::string & str)
    {
        m_buffer = str;
        m_lexeme.clear();
        m_next = 0;
        m_state = {};
        m_state.valid = true;
        m_state.wrap = true;
        m_backstep = {};
    }

    [[nodiscard]] auto empty() const -> bool
    {
        return m_next >= m_buffer.size();
    }

    [[nodiscard]] auto peek(size_t offset = 0) const -> int
    {
        if (m_next + offset < m_buffer.size()) {
            return m_buffer[m_next + offset];
        }
        return EOF;
    }

    auto advance() -> int
    {
        auto c{peek()};

        if (c == EOF) {
            return c;
        }

        m_lexeme.push_back(static_cast<char>(c));
        m_next++;
        m_backstep = m_state;

        if (m_state.wrap) {
            m_state.loc.line++;
            m_state.loc.column = 0;
        }

        m_state.loc.column++;
        m_state.wrap = (c == '\n');
        return c;
    }

    auto backup() -> bool
    {
        if (m_backstep.valid && !m_lexeme.empty()) {
            m_lexeme.pop_back();
            m_next--;

            m_state = m_backstep;
            m_backstep = {};
            return true;
        }

        return false;
    }

    auto lexeme() -> std::string &
    {
        return m_lexeme;
    }

    auto chain(std::function<size_t()> fn) -> Chain &
    {
        m_chain.fn_ = fn;
        m_chain.ret_ = fn();
        return m_chain;
    }

    struct State
    {
        bool valid;
        Loc loc;
        bool wrap;
    };

    std::string m_buffer;
    std::string m_lexeme;
    size_t m_next;
    State m_state;
    State m_backstep;
    Chain m_chain;
};


Scanner::Scanner() : pimpl{std::make_unique<Impl>(this)}
{
}

Scanner::~Scanner() = default;

void Scanner::assign(const std::string & str)
{
    pimpl->assign(str);
}

auto Scanner::position() const -> Scanner::Loc
{
    return pimpl->m_state.loc;
}

auto Scanner::empty() const -> bool
{
    return pimpl->empty();
}

auto Scanner::Chain::many() -> Scanner::Chain &
{
    for (;;) {
        auto n = fn_();
        if (!n) {
            break;
        }
        ret_ += n;
    }
    return *this;
}

auto Scanner::Chain::drop() -> size_t
{
    for (size_t n = 0; n < ret_; ++n) {
        s_->lexeme().pop_back();
    }
    return ret_;
}

// User-defined conversion function converts chain object to size_t.
Scanner::Chain::operator size_t() const
{
    return ret_;
}

auto Scanner::peek(size_t offset) const -> int
{
    return pimpl->peek(offset);
}

auto Scanner::advance() -> int
{
    return pimpl->advance();
}

auto Scanner::match(int c) -> Scanner::Chain &
{
    return pimpl->chain([&,c]() {
        if (c != peek()) {
            return 0;
        }
        pimpl->advance();
        return 1;
    });
}

auto Scanner::match(char const* set) -> Scanner::Chain &
{
    return pimpl->chain([&,set]() {
        if (!strchr(set, peek())) {
            return 0;
        }
        pimpl->advance();
        return 1;
    });
}

auto Scanner::match(const std::function<bool(int)> & predicate) -> Scanner::Chain &
{
    return pimpl->chain([&,predicate]() {
        if (!predicate(peek())) {
            return 0;
        }
        pimpl->advance();
        return 1;
    });
}

auto Scanner::match_sequence(char const *seq) -> Chain &
{
    return pimpl->chain([&,seq]() -> size_t {
        size_t off{};
        for (auto s = seq; *s; s++, off++) {
            if (peek(off) != *s) {
                return 0;
            }
        }
        for (size_t n{}; n < off; n++) {
            pimpl->advance();
        }
        return off;
    });
}

auto Scanner::backup() -> bool
{
    return pimpl->backup();
}

auto Scanner::lexeme() -> std::string &
{
    return pimpl->lexeme();
}


#ifdef UNITTEST_SCANNER

//@unittest clang++ -std=c++17 -Weverything -Wno-c++98-compat -Wno-padded -Wno-weak-vtables -Wno-poison-system-directories -DUNITTEST_SCANNER
int main()
{
    Scanner s;

    assert(s.empty());
    assert(s.lexeme().empty());
    assert(s.position().line == 0);
    assert(s.position().column == 0);
    assert(s.peek() == EOF);
    assert(s.advance() == EOF);

    s.assign("Abc\neeeffD ggh \t 358:");
    assert(!s.empty());
    assert(!s.backup());

    // Match individual characters.
    assert(1 == s.match('A'));

    // Backup one character.
    assert(s.position().line == 1);
    assert(s.position().column == 1);
    assert(s.backup());
    assert(s.position().line == 0);
    assert(s.position().column == 0);
    assert('A' == s.advance());
    assert(s.position().line == 1);
    assert(s.position().column == 1);

    assert(1 == s.match('b'));
    assert(0 == s.match('C'));
    assert(1 == s.match('c'));

    // The lexeme holds matched characters.
    assert(s.lexeme() == "Abc");
    s.lexeme().clear();

    // Backup over newline.
    assert('\n' == s.advance());
    assert(s.position().line == 1);
    assert(s.position().column == 4);
    assert(s.backup());
    assert(s.position().line == 1);
    assert(s.position().column == 3);
    assert('\n' == s.advance());
    assert(s.lexeme() == "\n");
    s.lexeme().clear();

    // Match one character from a set.
    assert(1 == s.match("D ef"));
    assert(s.position().line == 2);
    assert(s.position().column == 1);
    assert(s.backup());
    assert(s.position().line == 1);
    assert(s.position().column == 4);
    assert('e' == s.advance());
    assert(s.position().line == 2);
    assert(s.position().column == 1);

    // Use the chained 'many()' call to accept one or more characters.
    assert(6 == s.match("D ef").many());

    // Match a sequence of characters.
    assert(0 == s.match_sequence("ggH"));
    assert(0 == s.match_sequence("gghI"));
    assert(3 == s.match_sequence("ggh"));

    assert(s.lexeme() == "eeeffD ggh");
    s.lexeme().clear();

    // A matched sequence can be dropped (removed from the lexeme).
    assert(3 == s.match(" \t").many().drop());

    assert(1 == s.match([](int c){return isdigit(c);}));
    assert(2 == s.match([](int c){return isdigit(c);}).many());
    assert(s.lexeme() == "358");

    assert(1 == s.match(":"));
    assert(s.empty());
}

#endif
