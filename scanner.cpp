#include "scanner.hpp"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

class Scanner::Impl
{
public:
    explicit Impl(Scanner *parent) : buffer_{}, lexeme_{}, next_{}, state_{}, backstep_{}, chain_{parent}
    {
    }

    void assign(const std::string & str)
    {
        buffer_ = str;
        lexeme_.clear();
        next_ = 0;
        state_ = {};
        state_.valid = true;
        state_.wrap = true;
        backstep_ = {};
    }

    [[nodiscard]] auto empty() const -> bool
    {
        return next_ >= buffer_.size();
    }

    [[nodiscard]] auto peek(size_t offset = 0) const -> int
    {
        if (next_ + offset < buffer_.size()) {
            return buffer_[next_ + offset];
        }
        return EOF;
    }

    auto advance() -> int
    {
        auto c{peek()};

        if (c == EOF) {
            return c;
        }

        lexeme_.push_back(static_cast<char>(c));
        next_++;
        backstep_ = state_;

        if (state_.wrap) {
            state_.loc.line++;
            state_.loc.column = 0;
        }

        state_.loc.column++;
        state_.wrap = (c == '\n');
        return c;
    }

    auto backup() -> bool
    {
        if (backstep_.valid && !lexeme_.empty()) {
            lexeme_.pop_back();
            next_--;

            state_ = backstep_;
            backstep_ = {};
            return true;
        }

        return false;
    }

    auto lexeme() -> std::string &
    {
        return lexeme_;
    }

    auto chain(std::function<size_t()> fn) -> Chain &
    {
        chain_.fn_ = fn;
        chain_.ret_ = fn();
        return chain_;
    }

    struct State
    {
        bool valid;
        Loc loc;
        bool wrap;
    };

    std::string buffer_;
    std::string lexeme_;
    size_t next_;
    State state_;
    State backstep_;
    Chain chain_;
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
    return pimpl->state_.loc;
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
} // UNREACHABLE

auto Scanner::match(char const* set) -> Scanner::Chain &
{
    return pimpl->chain([&,set]() {
        if (!strchr(set, peek())) {
            return 0;
        }
        pimpl->advance();
        return 1;
    });
} // UNREACHABLE

auto Scanner::match(const std::function<bool(int)> & predicate) -> Scanner::Chain &
{
    return pimpl->chain([&,predicate]() {
        if (!predicate(peek())) {
            return 0;
        }
        pimpl->advance();
        return 1;
    });
} // UNREACHABLE

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
} // UNREACHABLE

auto Scanner::backup() -> bool
{
    return pimpl->backup();
}

auto Scanner::lexeme() -> std::string &
{
    return pimpl->lexeme();
}
