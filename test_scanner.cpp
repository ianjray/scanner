#include "scanner.hpp"

#include <cassert>

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
