#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <optional>
#include <cmath>
#include <random>
#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

constexpr double PI = 3.14159265358979323846;

// ============================================================
// AST base
// ============================================================

struct Expr {};

// ============================================================
// Values / Env
// ============================================================

using Value = double;

using Environment = std::unordered_map<std::string, Value>;

// ============================================================
// Distributions
// ============================================================

struct Normal { double mu, sigma; };
struct Bernoulli { double p; };

using Distribution = std::variant<Normal, Bernoulli>;

// ============================================================
// Closure
// ============================================================

struct Closure {
    std::vector<std::string> params;
    Expr* body;
    Environment env;
};

// ============================================================
// Instructions (CLEAN VERSION)
// ============================================================

struct Ev {
    Expr* e;
    Environment env;
    std::vector<std::string> addr;
};

struct Discard {};
struct CallK { int n; };
struct SampleK { std::vector<std::string> addr; };
struct ObserveK { std::vector<std::string> addr; };

using Instr = std::variant<
    Ev,
    Discard,
    CallK,
    SampleK,
    ObserveK
>;

// ============================================================
// Machine
// ============================================================

struct Machine {
    std::vector<Instr> C;
    std::vector<Value> V;
    Environment env;
    std::mt19937 rng;
    double log_w = 0.0;

    Machine() : rng(std::random_device{}()) {}
};

// ============================================================
// helpers
// ============================================================

double normal_logpdf(double x, double mu, double sigma) {
    double z = (x - mu) / sigma;
    return -0.5 * std::log(2 * PI)
           - std::log(sigma)
           - 0.5 * z * z;
}

double bernoulli_logpmf(bool x, double p) {
    return x ? std::log(p) : std::log(1.0 - p);
}

double log_prob(const Distribution& d, double x) {
    return std::visit([&](auto const& dist) {
        using T = std::decay_t<decltype(dist)>;

        if constexpr (std::is_same_v<T, Normal>)
            return normal_logpdf(x, dist.mu, dist.sigma);

        else
            return bernoulli_logpmf(x != 0, dist.p);

    }, d);
}

// ============================================================
// push_body (FIXED)
// ============================================================

void push_body(
    std::vector<Instr>& C,
    const std::vector<Expr*>& body,
    const Environment& env,
    std::vector<std::string> addr
) {
    std::vector<Instr> seq;

    for (size_t i = 0; i + 1 < body.size(); i++) {
        auto a = addr;
        a.push_back("body");
        a.push_back(std::to_string(i));

        seq.push_back(Ev{body[i], env, a});
        seq.push_back(Discard{});
    }

    if (!body.empty()) {
        auto a = addr;
        a.push_back("body");
        a.push_back(std::to_string(body.size() - 1));

        seq.push_back(Ev{body.back(), env, a});
    }

    for (auto it = seq.rbegin(); it != seq.rend(); ++it)
        C.push_back(*it);
}

// ============================================================
// resume (CLEAN VERSION)
// ============================================================

std::variant<std::string, Machine> resume(Machine& m) {

    while (!m.C.empty()) {

        Instr instr = std::move(m.C.back());
        m.C.pop_back();

        auto result = std::visit([&](auto&& op)
        -> std::optional<std::variant<std::string, Machine>>
        {
            using T = std::decay_t<decltype(op)>;

            // ---------------- EV ----------------
            if constexpr (std::is_same_v<T, Ev>) {
                m.V.push_back(0.0); // placeholder (AST no implementado aún)
            }

            // ---------------- DISCARD ----------------
            else if constexpr (std::is_same_v<T, Discard>) {
                m.V.pop_back();
            }

            // ---------------- CALL ----------------
            else if constexpr (std::is_same_v<T, CallK>) {
                return std::string("call");
            }

            // ---------------- SAMPLE ----------------
            else if constexpr (std::is_same_v<T, SampleK>) {
                return std::string("sample");
            }

            // ---------------- OBSERVE ----------------
            else if constexpr (std::is_same_v<T, ObserveK>) {
                return std::string("observe");
            }

            return std::nullopt;

        }, instr);

        if (result) {
            if (std::holds_alternative<Machine>(*result))
                return std::get<Machine>(*result);

            throw std::runtime_error("effect");
        }
    }

    return std::string("done");
}

// ============================================================
// MAIN
// ============================================================

int main() {


    return 0;
}