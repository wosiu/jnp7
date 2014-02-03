/*
 * JNP zadanie 7
 * Michał Woś mw336071
 */

#include <stack>
#include <string>
#include <functional>
#include <exception>
#include <cassert>

class SyntaxError : public std::exception {
    public:
        const char* what() const noexcept {
            return "SyntaxError";
        }
};

class OperatorAlreadyDefined : public std::exception {
    public:
        const char* what() const noexcept {
            return "OperatorAlreadyDefined";
        }
};

class UnknownOperator : public std::exception {
    public:
        const char* what() const noexcept {
            return "UnknownOperator";
        }
};

typedef std::function<int(void)> Lazy;

class LazyCalculator {
    private:
    	static const int S = (1<<(sizeof(char) * 8));
    	Lazy literals[S];
    	std::function<int(Lazy, Lazy)> operators[S];

    public:
        LazyCalculator() {
            literals[(int)'0'] = []() { return 0; };
            literals[(int)'2'] = []() { return 2; };
            literals[(int)'4'] = []() { return 4; };
            define('+', [](Lazy a, Lazy b) { return a() + b(); } );
            define('-', [](Lazy a, Lazy b) { return a() - b(); } );
            define('*', [](Lazy a, Lazy b) { return a() * b(); } );
            define('/', [](Lazy a, Lazy b) { return a() / b(); } );
        }

        Lazy parse(const std::string& s) const {
			std::stack<Lazy>onpStack;

            for (char c : s) {
            	if (literals[(int)c]) {
            		onpStack.push(literals[(int)c]);
				} else if (operators[(int)c]) {
                    if (onpStack.size() < 2) {
                        throw SyntaxError();
                    }
				    Lazy a = onpStack.top();
                    onpStack.pop();
                    Lazy b = onpStack.top();
                    onpStack.pop();
                    onpStack.push([=]() { return operators[(int)c](b, a); });
				} else {
				    throw UnknownOperator();
				}
            }

            if (onpStack.size() != 1) {
                throw SyntaxError();
            }
            return onpStack.top();
        }

        int calculate(const std::string& s) const {
            return parse(s)();
        }

        void define(char c, std::function<int(Lazy, Lazy)> fn) {
            if (operators[(int)c] || literals[(int)c]) {
                throw OperatorAlreadyDefined();
            }
            operators[(int)c] = fn;
        }
};

std::function<void(void)> operator*(int n, std::function<void(void)> fn) {
    return [=]() {
       for (int i = 0; i < n; i++)
           fn();
    };
}

int manytimes(Lazy n, Lazy fn) {
    (n() * fn)();  // Did you notice the type cast?
    return 0;
}

int main() {
    LazyCalculator calculator;

    // The only literals...
    assert(calculator.calculate("0") == 0);
    assert(calculator.calculate("2") == 2);
    assert(calculator.calculate("4") == 4);

    // Built-in operators.
    assert(calculator.calculate("42+") == 6);
    assert(calculator.calculate("24-") == -2);
    assert(calculator.calculate("42*") == 8);
    assert(calculator.calculate("42/") == 2);

    assert(calculator.calculate("42-2-") == 0);
    assert(calculator.calculate("242--") == 0);
    assert(calculator.calculate("22+2-2*2/0-") == 2);

    // The fun.
    calculator.define('!', [](Lazy a, Lazy b) { return a()*10 + b(); });
    assert(calculator.calculate("42!") == 42);

    std::string buffer;
    calculator.define(',', [](Lazy a, Lazy b) { a(); return b(); });
    calculator.define('P', [&buffer](Lazy a, Lazy b) { buffer += "pomidor"; return 0; });
    assert(calculator.calculate("42P42P42P42P42P42P42P42P42P42P42P42P42P42P42P42P,,,,42P42P42P42P42P,,,42P,42P,42P42P,,,,42P,,,42P,42P,42P,,42P,,,42P,42P42P42P42P42P42P42P42P,,,42P,42P,42P,,,,,,,,,,,,") == 0);
    assert(buffer.length() == 42 * std::string("pomidor").length());

    std::string buffer2 = std::move(buffer);
    buffer.clear();
    calculator.define('$', manytimes);
    assert(calculator.calculate("42!42P$") == 0);
    // Notice, how std::move worked.
    assert(buffer.length() == 42 * std::string("pomidor").length());

    calculator.define('?', [](Lazy a, Lazy b) { return a() ? b() : 0; });
    assert(calculator.calculate("042P?") == 0);
    assert(buffer == buffer2);

    assert(calculator.calculate("042!42P$?") == 0);
    assert(buffer == buffer2);

    for (auto bad: {"", "42", "4+", "424+"}) {
        try {
            calculator.calculate(bad);
            assert(false);
        } catch (SyntaxError) { }
    }

    try {
        calculator.define('!', [](Lazy a, Lazy b) { return a()*10 + b(); });
        assert(false);
    } catch (OperatorAlreadyDefined) { }

    try {
        calculator.calculate("02&");
        assert(false);
    } catch (UnknownOperator) { }
}
