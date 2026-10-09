#include "asterbook/io.hpp"
#include <iostream>
using namespace asterbook;
static const char *help = R"(AsterBook v0.1.0
  demo
  replay --input FILE --engine reference|price-level --events FILE --snapshot FILE
  verify --input FILE
  generate --seed N --count N --scenario add-only|cancel-heavy|mixed|sweep-heavy --levels N --output FILE
  bench --input FILE --engine reference|price-level|both --output FILE
Exit: 0 success, 2 usage, 3 I/O/parse/operational failure, 4 verification mismatch.
)";
int main(int argc, char **argv) {
    try {
        if (argc < 2) {
            std::cout << help;
            return 2;
        }
        std::string action = argv[1];
        if (action == "--help" || action == "help") {
            if (argc != 2)
                return 2;
            std::cout << help;
            return 0;
        }
        std::vector<std::string> allowed;
        if (action == "replay")
            allowed = {"--input", "--engine", "--events", "--snapshot"};
        else if (action == "verify")
            allowed = {"--input"};
        else if (action == "generate")
            allowed = {"--seed", "--count", "--scenario", "--levels", "--output"};
        else if (action == "bench")
            allowed = {"--input", "--engine", "--output"};
        else if (action != "demo") {
            std::cerr << "unknown command\n";
            return 2;
        }
        std::map<std::string, std::string> options;
        for (int i = 2; i < argc; i += 2) {
            std::string key = argv[i];
            if (i + 1 >= argc || std::find(allowed.begin(), allowed.end(), key) == allowed.end() ||
                options.contains(key)) {
                std::cerr << "invalid option: " << key << '\n';
                return 2;
            }
            options.emplace(key, argv[i + 1]);
        }
        auto get = [&](const std::string &key) -> std::string {
            auto it = options.find(key);
            if (it == options.end())
                throw std::invalid_argument("missing option: " + key);
            return it->second;
        };
        auto engine = [&]() {
            auto e = get("--engine");
            if (e != "reference" && e != "price-level" && !(action == "bench" && e == "both"))
                throw std::invalid_argument("invalid engine");
            return e;
        };
        if (action == "demo") {
            std::vector<Command> c = {{1, Action::Add, {1}, {1}, Side::Sell, {100}, {5}, Tif::GTC},
                                      {2, Action::Add, {1}, {2}, Side::Sell, {100}, {7}, Tif::GTC},
                                      {3, Action::Add, {1}, {3}, Side::Sell, {101}, {4}, Tif::GTC},
                                      {4, Action::Add, {1}, {1}, Side::Buy, {200}, {1}, Tif::GTC},
                                      {5, Action::Add, {1}, {4}, Side::Buy, {101}, {14}, Tif::GTC},
                                      {6, Action::Cancel, {1}, {3}},
                                      {7, Action::Add, {1}, {5}, Side::Buy, {99}, {3}, Tif::IOC},
                                      {8, Action::Add, {1}, {1}, Side::Buy, {98}, {2}, Tif::GTC}};
            auto r = replay(c, "price-level", true);
            std::cout
                << "FIFO: maker 1 then 2 at 100; sweep to 101; partial maker 3.\nDuplicate "
                   "rejected before matching; cancel remaining 2; IOC expires 3; ID 1 reused.\n"
                << events_csv(r.events) << r.snapshot;
            return 0;
        }
        if (action == "generate") {
            auto levels = options.contains("--levels") ? number(get("--levels")) : 32;
            auto count = number(get("--count"));
            if (count > 10000000)
                throw std::invalid_argument("count exceeds safety limit 10000000");
            auto c = generate(number(get("--seed")), static_cast<std::size_t>(count),
                              get("--scenario"), static_cast<std::size_t>(levels));
            write_commands(get("--output"), c);
            std::cout << "generated " << c.size() << " commands\n";
            return 0;
        }
        auto input = get("--input");
        std::string e;
        if (action != "verify")
            e = engine();
        if (action == "replay") {
            get("--events");
            get("--snapshot");
        } else if (action == "bench")
            get("--output");
        auto commands = read_commands(input);
        if (action == "verify") {
            if (replay(commands, "reference", true) != replay(commands, "price-level", true)) {
                std::cerr << "verification mismatch\n";
                return 4;
            }
            std::cout << "verified " << commands.size() << " commands\n";
        } else if (action == "replay") {
            auto r = replay(commands, e);
            write_text(get("--events"), events_csv(r.events));
            write_text(get("--snapshot"), r.snapshot);
            std::cout << "replayed " << commands.size() << " commands\n";
        } else if (action == "bench") {
            bench(commands, e, get("--output"));
            std::cout << "benchmark saved\n";
        }
        return 0;
    } catch (const std::invalid_argument &e) {
        std::cerr << e.what() << '\n';
        return 2;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 3;
    }
}
