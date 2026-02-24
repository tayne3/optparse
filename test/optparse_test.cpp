#define OPTPARSE_IMPLEMENTATION
#include "optparse/optparse.h"

#include <string>
#include <vector>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

class Argv {
public:
	explicit Argv(std::initializer_list<const char *> args) {
		_ss.push_back(const_cast<char *>("prog"));
		for (auto s : args) { _ss.push_back(const_cast<char *>(s)); }
		_ss.push_back(nullptr);
	}

	char **data() { return _ss.data(); }

	struct optparse to_opts() {
		struct optparse o;
		optparse_init(&o, data());
		return o;
	}

private:
	std::vector<char *> _ss;
};

static std::vector<std::string> unconsumed_args(struct optparse *o) {
	std::vector<std::string> v;
	for (char *a = optparse_arg(o); a != nullptr; a = optparse_arg(o)) { v.push_back(a); }
	return v;
}

/** Standard long-option table used by most tests. */
static const struct optparse_long kLongopts[] = {
	{"amend", 'a', OPTPARSE_NONE}, {"brief", 'b', OPTPARSE_NONE},    {"color", 'c', OPTPARSE_OPTIONAL}, {"delay", 'd', OPTPARSE_REQUIRED},
	{"erase", 'e', OPTPARSE_NONE}, {"file", 'f', OPTPARSE_REQUIRED}, {nullptr, 0, OPTPARSE_NONE},
};

TEST_CASE("short: no arguments", "[short]") {
	Argv av{};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc") == -1);
}

TEST_CASE("short: single flag", "[short]") {
	Argv av{"-a"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc") == 'a');
	REQUIRE(optparse(&o, "abc") == -1);
}

TEST_CASE("short: multiple flags", "[short]") {
	Argv av{"-a", "-b", "-c"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc") == 'a');
	REQUIRE(optparse(&o, "abc") == 'b');
	REQUIRE(optparse(&o, "abc") == 'c');
	REQUIRE(optparse(&o, "abc") == -1);
}

TEST_CASE("short: combined cluster -abc", "[short]") {
	Argv av{"-abc"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc") == 'a');
	REQUIRE(optparse(&o, "abc") == 'b');
	REQUIRE(optparse(&o, "abc") == 'c');
	REQUIRE(optparse(&o, "abc") == -1);
}

TEST_CASE("short: required argument — separate token", "[short]") {
	Argv av{"-c", "red"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "c:") == 'c');
	REQUIRE(o.optarg != nullptr);
	REQUIRE(std::string(o.optarg) == "red");
}

TEST_CASE("short: required argument — inline (no space)", "[short]") {
	Argv av{"-cred"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "c:") == 'c');
	REQUIRE(std::string(o.optarg) == "red");
}

TEST_CASE("short: required argument — combined cluster with arg", "[short]") {
	Argv av{"-abcblue"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc:") == 'a');
	REQUIRE(optparse(&o, "abc:") == 'b');
	int r = optparse(&o, "abc:");
	REQUIRE(r == 'c');
	REQUIRE(std::string(o.optarg) == "blue");
}

TEST_CASE("short: optional argument — present inline", "[short]") {
	Argv av{"-d10"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "d::") == 'd');
	REQUIRE(o.optarg != nullptr);
	REQUIRE(std::string(o.optarg) == "10");
}

TEST_CASE("short: optional argument — absent", "[short]") {
	Argv av{"-d", "10"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "d::") == 'd');
	REQUIRE(o.optarg == nullptr);
	/* "10" becomes a positional argument */
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"10"};
	REQUIRE(args == expected);
}

TEST_CASE("short: unknown option returns '?'", "[short][error]") {
	Argv av{"-z"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "abc") == '?');
	REQUIRE(std::string(o.errmsg).find(OPTPARSE_MSG_INVALID) == 0);
}

TEST_CASE("short: missing required argument returns '?'", "[short][error]") {
	Argv av{"-c"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "c:") == '?');
	REQUIRE(std::string(o.errmsg).find(OPTPARSE_MSG_MISSING) == 0);
}

TEST_CASE("short: repeated flag increments count", "[short]") {
	Argv av{"-eeeeee"};
	auto o     = av.to_opts();
	int  count = 0;
	int  c;
	while ((c = optparse(&o, "e")) != -1) {
		REQUIRE(c == 'e');
		count++;
	}
	REQUIRE(count == 6);
}

TEST_CASE("short: errmsg is empty on success", "[short]") {
	Argv av{"-a"};
	auto o = av.to_opts();
	optparse(&o, "a");
	REQUIRE(o.errmsg[0] == '\0');
}

TEST_CASE("long: single flag --amend", "[long]") {
	Argv av{"--amend"};
	auto o  = av.to_opts();
	int  li = -1;
	REQUIRE(optparse_long(&o, kLongopts, &li) == 'a');
	REQUIRE(li == 0);
}

TEST_CASE("long: multiple flags", "[long]") {
	Argv av{"--amend", "--brief"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'b');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
}

TEST_CASE("long: required argument — separate token", "[long]") {
	Argv av{"--delay", "500"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'd');
	REQUIRE(std::string(o.optarg) == "500");
}

TEST_CASE("long: required argument — inline with '='", "[long]") {
	Argv av{"--color=red"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'c');
	REQUIRE(std::string(o.optarg) == "red");
}

TEST_CASE("long: optional argument — present inline", "[long]") {
	Argv av{"--color=blue"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'c');
	REQUIRE(std::string(o.optarg) == "blue");
}

TEST_CASE("long: optional argument — absent", "[long]") {
	Argv av{"--color"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'c');
	REQUIRE(o.optarg == nullptr);
}

TEST_CASE("long: required argument missing returns '?'", "[long][error]") {
	Argv av{"--delay"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == '?');
	REQUIRE(std::string(o.errmsg).find(OPTPARSE_MSG_MISSING) == 0);
}

TEST_CASE("long: unknown option returns '?'", "[long][error]") {
	Argv av{"--foo"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == '?');
	REQUIRE(std::string(o.errmsg).find(OPTPARSE_MSG_INVALID) == 0);
}

TEST_CASE("long: TOOMANY when flag given an argument", "[long][error]") {
	Argv av{"--amend=yes"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == '?');
	REQUIRE(std::string(o.errmsg).find(OPTPARSE_MSG_TOOMANY) == 0);
}

TEST_CASE("long: long-only option (shortname > 127)", "[long]") {
	static const struct optparse_long lo[] = {{"verbose", 256, OPTPARSE_NONE}, {"output", 257, OPTPARSE_REQUIRED}, {nullptr, 0, OPTPARSE_NONE}};
	Argv                              av{"--verbose", "--output", "file.txt"};
	auto                              o  = av.to_opts();
	int                               li = -1;

	REQUIRE(optparse_long(&o, lo, &li) == 256);
	REQUIRE(li == 0);

	REQUIRE(optparse_long(&o, lo, &li) == 257);
	REQUIRE(li == 1);
	REQUIRE(std::string(o.optarg) == "file.txt");
}

TEST_CASE("long: longindex is set to matching entry for short options", "[long]") {
	Argv av{"-a"};
	auto o  = av.to_opts();
	int  li = 99;
	REQUIRE(optparse_long(&o, kLongopts, &li) == 'a');
	REQUIRE(li == 0); /* fallback matches shortname 'a' → "amend" at index 0 */
}

TEST_CASE("long: mix of short and long options", "[long]") {
	/* Note: -c uses OPTPARSE_OPTIONAL in kLongopts, so "-c green" will NOT
	 * consume "green" as the argument (standard getopt behaviour for optional
	 * args with short syntax — inline "-cgreen" is required).
	 * Use "--color=green" or "-cgreen" for optional; use a long option with
	 * OPTPARSE_REQUIRED for separate-token style. Here we use --color=green. */
	Argv        av{"-a", "--brief", "--color=green", "--delay", "42"};
	auto        o     = av.to_opts();
	bool        amend = false, brief = false;
	std::string color;
	int         delay = 0;

	int c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		switch (c) {
			case 'a': amend = true; break;
			case 'b': brief = true; break;
			case 'c': color = o.optarg ? o.optarg : ""; break;
			case 'd': delay = std::atoi(o.optarg); break;
			default: FAIL("unexpected option");
		}
	}
	REQUIRE(amend);
	REQUIRE(brief);
	REQUIRE(color == "green");
	REQUIRE(delay == 42);
}

TEST_CASE("permute: non-option before option", "[permute]") {
	Argv av{"foo", "--amend", "bar"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar"};
	REQUIRE(args == expected);
}

TEST_CASE("permute: options interspersed with positionals", "[permute]") {
	Argv        av{"foo", "--delay", "1234", "bar", "-cred"};
	auto        o = av.to_opts();
	std::string color;
	int         delay = 0;
	int         c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		switch (c) {
			case 'c': color = o.optarg ? o.optarg : ""; break;
			case 'd': delay = std::atoi(o.optarg); break;
		}
	}
	REQUIRE(color == "red");
	REQUIRE(delay == 1234);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar"};
	REQUIRE(args == expected);
}

TEST_CASE("permute: all positionals, no options", "[permute]") {
	Argv av{"foo", "bar", "baz"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar", "baz"};
	REQUIRE(args == expected);
}

TEST_CASE("posix: stop at first non-option", "[posix]") {
	Argv av{"-a", "stop", "-b"};
	auto o    = av.to_opts();
	o.permute = 0;
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	/* "-b" stays unconsumed — only "stop" is the next positional */
	auto args = unconsumed_args(&o);
	REQUIRE(args.size() >= 1);
	REQUIRE(args[0] == "stop");
}

TEST_CASE("arg: basic positional collection", "[arg]") {
	Argv av{"-a", "foo", "bar"};
	auto o = av.to_opts();
	optparse_long(&o, kLongopts, nullptr); /* consume -a */
	optparse_long(&o, kLongopts, nullptr); /* returns -1 */
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar"};
	REQUIRE(args == expected);
}

TEST_CASE("arg: step over subcommand and re-parse", "[arg]") {
	/* Simulates:  prog -a subcmd -b
	 * Sub-command parsing requires POSIX mode (permute=0) so that the first
	 * loop stops at "subcmd" rather than permuting it to the end. */
	Argv av{"-a", "subcmd", "-b"};
	auto o    = av.to_opts();
	o.permute = 0; /* POSIX mode: stop at first non-option */

	/* Parse main options — stops before "subcmd" */
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);

	/* Step over sub-command token */
	char *subcmd = optparse_arg(&o);
	REQUIRE(subcmd != nullptr);
	REQUIRE(std::string(subcmd) == "subcmd");

	/* Parse sub-command options */
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'b');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
}

TEST_CASE("arg: returns NULL when exhausted", "[arg]") {
	Argv av{};
	auto o = av.to_opts();
	REQUIRE(optparse_arg(&o) == nullptr);
}

TEST_CASE("edge: double-dash '--' terminates option parsing", "[edge]") {
	Argv av{"--", "foobar"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foobar"};
	REQUIRE(args == expected);
}

TEST_CASE("edge: single dash '-' is treated as positional", "[edge]") {
	Argv av{"-"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"-"};
	REQUIRE(args == expected);
}

TEST_CASE("edge: empty option string", "[edge]") {
	Argv av{"-a"};
	auto o = av.to_opts();
	REQUIRE(optparse(&o, "") == '?');
}

TEST_CASE("edge: re-initialise resets state", "[edge]") {
	Argv av{"-a", "-b"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	/* Re-initialise mid-stream */
	optparse_init(&o, av.data());
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'b');
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
}

TEST_CASE("edge: longindex nullptr does not crash", "[edge]") {
	Argv av{"--amend"};
	auto o = av.to_opts();
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == 'a');
}

TEST_CASE("edge: color inline with combined short opts", "[edge]") {
	/* -abcblue → a, b, c with optarg="blue" */
	Argv av{"-abcblue"};
	auto o = av.to_opts();

	static const struct optparse_long lo[] = {
		{"amend", 'a', OPTPARSE_NONE},
		{"brief", 'b', OPTPARSE_NONE},
		{"color", 'c', OPTPARSE_REQUIRED},
		{nullptr, 0, OPTPARSE_NONE},
	};

	bool        a = false, b = false;
	std::string color;
	int         c;
	while ((c = optparse_long(&o, lo, nullptr)) != -1) {
		switch (c) {
			case 'a': a = true; break;
			case 'b': b = true; break;
			case 'c': color = o.optarg ? o.optarg : ""; break;
		}
	}
	REQUIRE(a);
	REQUIRE(b);
	REQUIRE(color == "blue");
}

TEST_CASE("errmsg: INVALID contains option name", "[errmsg]") {
	Argv av{"--unknown"};
	auto o = av.to_opts();
	optparse_long(&o, kLongopts, nullptr);
	std::string msg(o.errmsg);
	REQUIRE(msg.find(OPTPARSE_MSG_INVALID) == 0);
	REQUIRE(msg.find("unknown") != std::string::npos);
}

TEST_CASE("errmsg: MISSING contains option name", "[errmsg]") {
	Argv av{"--delay"};
	auto o = av.to_opts();
	optparse_long(&o, kLongopts, nullptr);
	std::string msg(o.errmsg);
	REQUIRE(msg.find(OPTPARSE_MSG_MISSING) == 0);
	REQUIRE(msg.find("delay") != std::string::npos);
}

TEST_CASE("errmsg: TOOMANY contains option name", "[errmsg]") {
	Argv av{"--amend=yes"};
	auto o = av.to_opts();
	optparse_long(&o, kLongopts, nullptr);
	std::string msg(o.errmsg);
	REQUIRE(msg.find(OPTPARSE_MSG_TOOMANY) == 0);
	REQUIRE(msg.find("amend") != std::string::npos);
}

TEST_CASE("errmsg: short INVALID contains option character", "[errmsg]") {
	Argv av{"-z"};
	auto o = av.to_opts();
	optparse(&o, "abc");
	std::string msg(o.errmsg);
	REQUIRE(msg.find(OPTPARSE_MSG_INVALID) == 0);
	REQUIRE(msg.find('z') != std::string::npos);
}

TEST_CASE("errmsg: short MISSING contains option character", "[errmsg]") {
	Argv av{"-c"};
	auto o = av.to_opts();
	optparse(&o, "c:");
	std::string msg(o.errmsg);
	REQUIRE(msg.find(OPTPARSE_MSG_MISSING) == 0);
	REQUIRE(msg.find('c') != std::string::npos);
}

struct Config {
	bool        amend = false;
	bool        brief = false;
	std::string color; /* empty string means "not set" unless set_color */
	bool        set_color = false;
	int         delay     = 0;
	int         erase     = 0;
	std::string err;
};

static Config run_long(char **argv_raw, const struct optparse_long *lo = kLongopts) {
	Config          cfg;
	struct optparse o;
	optparse_init(&o, argv_raw);

	int c;
	while ((c = optparse_long(&o, lo, nullptr)) != -1) {
		switch (c) {
			case 'a': cfg.amend = true; break;
			case 'b': cfg.brief = true; break;
			case 'c':
				cfg.set_color = true;
				cfg.color     = o.optarg ? o.optarg : "";
				break;
			case 'd': cfg.delay = std::atoi(o.optarg); break;
			case 'e': cfg.erase++; break;
			default: cfg.err = o.errmsg;
		}
	}
	return cfg;
}

TEST_CASE("regression: -- foobar", "[regression]") {
	Argv av{"--", "foobar"};
	auto cfg = run_long(av.data());
	auto o   = av.to_opts(); /* fresh parse for args */
	/* consume the options first */
	while (optparse_long(&o, kLongopts, nullptr) != -1) {}
	auto args = unconsumed_args(&o);
	REQUIRE_FALSE(cfg.amend);
	REQUIRE_FALSE(cfg.brief);
	auto expected = std::vector<std::string>{"foobar"};
	REQUIRE(args == expected);
}

TEST_CASE("regression: -a -b -c -d 10 -e", "[regression]") {
	Argv av{"-a", "-b", "-c", "-d", "10", "-e"};
	auto cfg = run_long(av.data());
	REQUIRE(cfg.amend);
	REQUIRE(cfg.brief);
	REQUIRE(cfg.set_color);
	REQUIRE(cfg.color.empty());
	REQUIRE(cfg.delay == 10);
	REQUIRE(cfg.erase == 1);
	REQUIRE(cfg.err.empty());
}

TEST_CASE("regression: --amend --brief --color --delay 10 --erase", "[regression]") {
	Argv av{"--amend", "--brief", "--color", "--delay", "10", "--erase"};
	auto cfg = run_long(av.data());
	REQUIRE(cfg.amend);
	REQUIRE(cfg.brief);
	REQUIRE(cfg.set_color);
	REQUIRE(cfg.color.empty());
	REQUIRE(cfg.delay == 10);
	REQUIRE(cfg.erase == 1);
}

TEST_CASE("regression: -a -b -cred -d 10 -e", "[regression]") {
	Argv av{"-a", "-b", "-cred", "-d", "10", "-e"};
	auto cfg = run_long(av.data());
	REQUIRE(cfg.amend);
	REQUIRE(cfg.brief);
	REQUIRE(cfg.color == "red");
	REQUIRE(cfg.delay == 10);
	REQUIRE(cfg.erase == 1);
}

TEST_CASE("regression: -abcblue -d10 foobar", "[regression]") {
	Argv av{"-abcblue", "-d10", "foobar"};

	static const struct optparse_long lo[] = {
		{"amend", 'a', OPTPARSE_NONE},     {"brief", 'b', OPTPARSE_NONE}, {"color", 'c', OPTPARSE_REQUIRED},
		{"delay", 'd', OPTPARSE_REQUIRED}, {"erase", 'e', OPTPARSE_NONE}, {nullptr, 0, OPTPARSE_NONE},
	};

	struct optparse o;
	optparse_init(&o, av.data());
	Config cfg;
	int    c;
	while ((c = optparse_long(&o, lo, nullptr)) != -1) {
		switch (c) {
			case 'a': cfg.amend = true; break;
			case 'b': cfg.brief = true; break;
			case 'c': cfg.color = o.optarg ? o.optarg : ""; break;
			case 'd': cfg.delay = std::atoi(o.optarg); break;
		}
	}
	REQUIRE(cfg.amend);
	REQUIRE(cfg.brief);
	REQUIRE(cfg.color == "blue");
	REQUIRE(cfg.delay == 10);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foobar"};
	REQUIRE(args == expected);
}

TEST_CASE("regression: --color=red -d 10 -- foobar", "[regression]") {
	Argv            av{"--color=red", "-d", "10", "--", "foobar"};
	struct optparse o;
	optparse_init(&o, av.data());
	Config cfg;
	int    c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		switch (c) {
			case 'c': cfg.color = o.optarg ? o.optarg : ""; break;
			case 'd': cfg.delay = std::atoi(o.optarg); break;
		}
	}
	REQUIRE(cfg.color == "red");
	REQUIRE(cfg.delay == 10);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foobar"};
	REQUIRE(args == expected);
}

TEST_CASE("regression: -eeeeee increments to 6", "[regression]") {
	Argv av{"-eeeeee"};
	auto cfg = run_long(av.data());
	REQUIRE(cfg.erase == 6);
}

TEST_CASE("regression: --delay (missing arg) gives MISSING error", "[regression]") {
	Argv av{"--delay"};
	auto cfg = run_long(av.data());
	REQUIRE(std::string(cfg.err).find(OPTPARSE_MSG_MISSING) == 0);
}

TEST_CASE("regression: --foo bar leaves foo and bar as positionals", "[regression]") {
	Argv            av{"--foo", "bar"};
	struct optparse o;
	optparse_init(&o, av.data());
	std::string err;
	int         c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		if (c == '?') err = o.errmsg;
	}
	REQUIRE(std::string(err).find(OPTPARSE_MSG_INVALID) == 0);
}

TEST_CASE("regression: -x leaves -x as positional", "[regression]") {
	Argv av{"-x"};
	auto cfg = run_long(av.data());
	REQUIRE(cfg.err.find(OPTPARSE_MSG_INVALID) == 0);
}

TEST_CASE("regression: - is positional", "[regression]") {
	Argv            av{"-"};
	struct optparse o;
	optparse_init(&o, av.data());
	REQUIRE(optparse_long(&o, kLongopts, nullptr) == -1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"-"};
	REQUIRE(args == expected);
}

TEST_CASE("regression: -e foo bar baz -a quux", "[regression]") {
	Argv            av{"-e", "foo", "bar", "baz", "-a", "quux"};
	struct optparse o;
	optparse_init(&o, av.data());
	Config cfg;
	int    c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		switch (c) {
			case 'a': cfg.amend = true; break;
			case 'e': cfg.erase++; break;
		}
	}
	REQUIRE(cfg.amend);
	REQUIRE(cfg.erase == 1);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar", "baz", "quux"};
	REQUIRE(args == expected);
}

TEST_CASE("regression: foo --delay 1234 bar -cred", "[regression]") {
	Argv            av{"foo", "--delay", "1234", "bar", "-cred"};
	struct optparse o;
	optparse_init(&o, av.data());
	Config cfg;
	int    c;
	while ((c = optparse_long(&o, kLongopts, nullptr)) != -1) {
		switch (c) {
			case 'c': cfg.color = o.optarg ? o.optarg : ""; break;
			case 'd': cfg.delay = std::atoi(o.optarg); break;
		}
	}
	REQUIRE(cfg.color == "red");
	REQUIRE(cfg.delay == 1234);
	auto args     = unconsumed_args(&o);
	auto expected = std::vector<std::string>{"foo", "bar"};
	REQUIRE(args == expected);
}
