#pragma once

#include <functional>
#include <memory>
#include <string>

/// @class Scanner.
/// Scanner class.
/// @discussion Supports the scanning of lexemes.
class Scanner
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// Describes location in source text.
    struct Loc
    {
        size_t line;
        size_t column;
    };

    /// Class Chain.
    /// @discussion
    /// Certain @c Scanner operations return a reference to the @c Chain
    /// object, and this allows chained function calls.
    /// @see match
    class Chain
    {
    public:
        explicit Chain(Scanner *s) : s_{s}, fn_{}, ret_{} {}

        // Disallow copying since this object contains pointers.
        Chain(const Chain &) = delete;
        auto operator=(const Chain &) -> Chain & = delete;
        Chain(Chain&& other) = delete;
        auto operator=(Chain &&) -> Chain & = delete;
        virtual ~Chain() = default;

        /// Repeat until no more matches occur.
        /// @return Chain& Reference to @c Scanner::Chain.
        auto many() -> Chain &;

        /// Drop characters.
        /// @discussion Drop matched characters from the lexeme.
        /// @return size_t Number of dropped characters.
        auto drop() -> size_t;

        /// Obtain result of operation.
        /// @return size_t Number of matched characters.
        operator size_t() const;

    private:
        Scanner * s_;
        std::function<size_t()> fn_;
        size_t ret_;
        friend class Scanner;
    };

    /// Constructor
    Scanner();

    Scanner(const Scanner &) = delete;
    auto operator=(const Scanner &) -> Scanner & = delete;
    Scanner(Scanner&& other) = delete;
    auto operator=(Scanner &&) -> Scanner & = delete;
    ~Scanner();

    /// Assign content to be scanned.
    void assign(const std::string & str);

    /// Get position.
    /// @return Loc The current position.
    [[nodiscard]] auto position() const -> Loc;

    /// Test if content is empty.
    /// @return bool Returns whether the content is empty.
    [[nodiscard]] auto empty() const -> bool;

    /// @return int Current character (without advancing position).
    [[nodiscard]] auto peek(size_t offset = 0) const -> int;

    /// Advance to next character.
    /// @discussion Add current character to lexeme and advance to next character.
    /// @return int Character.
    auto advance() -> int;

    /// Match character.
    /// @discussion If current character matches @c c, then add it to the lexeme, and advance.
    /// @return Chain& Reference to @c Scanner::Chain.
    auto match(int c) -> Chain &;

    /// Match character from set.
    /// @discussion If current character is a member of the @c set, then add it to the lexeme, and advance.
    /// @return Chain& Reference to @c Scanner::Chain.
    auto match(char const *set) -> Chain &;

    /// Accept character matched by predicate.
    /// @discussion If current character satisfies the @c predicate, then add it to the lexeme, and advance.
    /// @return Chain& Reference to @c Scanner::Chain.
    auto match(const std::function<bool(int)> & predicate) -> Chain &;

    /// Match sequence of characters.
    /// @discussion If whole sequence matches then add to the lexeme, and advance.
    /// @return Chain& Reference to @c Scanner::Chain.
    auto match_sequence(char const *seq) -> Chain &;

    /// Back-step one character.
    /// @return bool True if back-stepping succeeded.
    /// @note Only one level of back-stepping is supported.
    auto backup() -> bool;

    /// Lexeme accessor.
    /// @return std::string Lexeme.
    /// @note Use std:move to take ownership of the lexeme.
    auto lexeme() -> std::string &;
};
