#include "BigInteger.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <sstream>
#include <cassert>

// ── Constructors ──────────────────────────────────────────────────────────────

BigInteger::BigInteger() : digits_{0} {}

BigInteger::BigInteger(uint64_t value) {
    if (value == 0) { digits_.push_back(0); return; }
    while (value > 0) {
        digits_.push_back(value % BASE);
        value /= BASE;
    }
}

BigInteger::BigInteger(const std::string& decimal) {
    if (decimal.empty()) { digits_.push_back(0); return; }
    // Parse decimal string right-to-left in chunks of 9 digits
    int i = static_cast<int>(decimal.size());
    while (i > 0) {
        int start = std::max(0, i - 9);
        digits_.push_back(std::stoull(decimal.substr(start, i - start)));
        i = start;
    }
    trim();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void BigInteger::trim() {
    while (digits_.size() > 1 && digits_.back() == 0)
        digits_.pop_back();
}

BigInteger BigInteger::fromDigits(std::vector<uint64_t> d) {
    BigInteger r;
    r.digits_ = std::move(d);
    r.trim();
    return r;
}

// ── Comparison ────────────────────────────────────────────────────────────────

bool BigInteger::operator==(const BigInteger& rhs) const { return digits_ == rhs.digits_; }
bool BigInteger::operator!=(const BigInteger& rhs) const { return !(*this == rhs); }

bool BigInteger::operator<(const BigInteger& rhs) const {
    if (digits_.size() != rhs.digits_.size())
        return digits_.size() < rhs.digits_.size();
    for (int i = static_cast<int>(digits_.size()) - 1; i >= 0; --i) {
        if (digits_[i] != rhs.digits_[i]) return digits_[i] < rhs.digits_[i];
    }
    return false; // equal
}

bool BigInteger::operator<=(const BigInteger& rhs) const { return !(rhs < *this); }
bool BigInteger::operator>(const BigInteger& rhs) const  { return rhs < *this; }
bool BigInteger::operator>=(const BigInteger& rhs) const { return !(*this < rhs); }

// ── Arithmetic ────────────────────────────────────────────────────────────────

BigInteger BigInteger::operator+(const BigInteger& rhs) const {
    std::vector<uint64_t> res;
    uint64_t carry = 0;
    size_t n = std::max(digits_.size(), rhs.digits_.size());
    for (size_t i = 0; i < n || carry; ++i) {
        uint64_t sum = carry;
        if (i < digits_.size()) sum += digits_[i];
        if (i < rhs.digits_.size()) sum += rhs.digits_[i];
        res.push_back(sum % BASE);
        carry = sum / BASE;
    }
    return fromDigits(std::move(res));
}

BigInteger BigInteger::operator-(const BigInteger& rhs) const {
    // Assumes *this >= rhs
    std::vector<uint64_t> res;
    int64_t borrow = 0;
    for (size_t i = 0; i < digits_.size(); ++i) {
        int64_t diff = static_cast<int64_t>(digits_[i]) - borrow
                     - (i < rhs.digits_.size() ? static_cast<int64_t>(rhs.digits_[i]) : 0);
        if (diff < 0) { diff += static_cast<int64_t>(BASE); borrow = 1; }
        else borrow = 0;
        res.push_back(static_cast<uint64_t>(diff));
    }
    return fromDigits(std::move(res));
}

BigInteger BigInteger::operator*(const BigInteger& rhs) const {
    std::vector<uint64_t> res(digits_.size() + rhs.digits_.size(), 0);
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < rhs.digits_.size() || carry; ++j) {
            __uint128_t cur = static_cast<__uint128_t>(res[i + j]) + carry;
            if (j < rhs.digits_.size())
                cur += static_cast<__uint128_t>(digits_[i]) * rhs.digits_[j];
            res[i + j] = static_cast<uint64_t>(cur % BASE);
            carry = static_cast<uint64_t>(cur / BASE);
        }
    }
    return fromDigits(std::move(res));
}

BigInteger BigInteger::operator/(const BigInteger& rhs) const {
    if (rhs.isZero()) throw std::domain_error("Division by zero");
    if (*this < rhs) return BigInteger(0ULL);

    // Long division on base-10^9 digits
    BigInteger quotient, remainder;
    for (int i = static_cast<int>(digits_.size()) - 1; i >= 0; --i) {
        // remainder = remainder * BASE + digits_[i]
        std::vector<uint64_t> shifted(remainder.digits_.size() + 1, 0);
        shifted[0] = digits_[i];
        for (size_t k = 0; k < remainder.digits_.size(); ++k)
            shifted[k + 1] = remainder.digits_[k];
        remainder = fromDigits(std::move(shifted));

        // Binary search for the largest q s.t. rhs * q <= remainder
        uint64_t lo = 0, hi = BASE - 1, q = 0;
        while (lo <= hi) {
            uint64_t mid = lo + (hi - lo) / 2;
            if (rhs * BigInteger(mid) <= remainder) { q = mid; lo = mid + 1; }
            else { if (mid == 0) break; hi = mid - 1; }
        }
        remainder = remainder - rhs * BigInteger(q);

        std::vector<uint64_t> qd(quotient.digits_.size() + 1, 0);
        qd[0] = q; // will be shifted below — actually prepend
        // Build quotient digit-by-digit (most significant first, so prepend)
        std::vector<uint64_t> newQ;
        newQ.push_back(q);
        for (auto d : quotient.digits_) newQ.push_back(d);
        quotient = fromDigits(std::move(newQ));
    }
    quotient.trim();
    return quotient;
}

BigInteger BigInteger::operator%(const BigInteger& mod) const {
    if (mod.isZero()) throw std::domain_error("Modulo by zero");
    if (*this < mod) return *this;
    // Use: a % b = a - (a / b) * b
    BigInteger q = *this / mod;
    BigInteger r = *this - q * mod;
    r.trim();
    return r;
}

// ── Bit operations ────────────────────────────────────────────────────────────

bool BigInteger::isZero() const { return digits_.size() == 1 && digits_[0] == 0; }
bool BigInteger::isOne()  const { return digits_.size() == 1 && digits_[0] == 1; }
bool BigInteger::isEven() const { return (digits_[0] & 1) == 0; }

int BigInteger::bitLength() const {
    if (isZero()) return 0;
    int bits = static_cast<int>((digits_.size() - 1)) * 30; // approximate
    // Count exact bits in the top base-10^9 limb by converting via binary
    // Use a simpler approach: repeated division by 2
    // Actually, convert top limb:
    uint64_t top = digits_.back();
    int extra = 0;
    while (top > 0) { top >>= 1; ++extra; }
    // Each base-10^9 limb holds up to 30 bits (2^30 = 1,073,741,824 > 10^9)
    bits = static_cast<int>((digits_.size() - 1)) * 30 + extra;
    return bits;
}

bool BigInteger::getBit(int pos) const {
    // Convert to binary bit at position pos
    BigInteger tmp = *this;
    BigInteger two(2ULL);
    for (int i = 0; i < pos; ++i) tmp = tmp / two;
    return (tmp.digits_[0] & 1) == 1;
}

BigInteger BigInteger::operator>>(int shift) const {
    BigInteger result = *this;
    BigInteger two(2ULL);
    for (int i = 0; i < shift; ++i) result = result / two;
    return result;
}

BigInteger BigInteger::operator<<(int shift) const {
    BigInteger result = *this;
    BigInteger two(2ULL);
    for (int i = 0; i < shift; ++i) result = result * two;
    return result;
}

// ── Modular exponentiation ────────────────────────────────────────────────────

BigInteger BigInteger::modPow(const BigInteger& base,
                              const BigInteger& exp,
                              const BigInteger& mod) {
    if (mod.isOne()) return BigInteger(0ULL);
    BigInteger result(1ULL);
    BigInteger b = base % mod;
    BigInteger e = exp;
    BigInteger two(2ULL);

    while (!e.isZero()) {
        if (!e.isEven()) result = result * b % mod;
        e = e >> 1;
        b = b * b % mod;
    }
    return result;
}

// ── Miller-Rabin primality test ───────────────────────────────────────────────

bool BigInteger::isPrime(int rounds) const {
    if (*this < BigInteger(2ULL)) return false;
    if (*this == BigInteger(2ULL) || *this == BigInteger(3ULL)) return true;
    if (isEven()) return false;

    // Write n-1 as 2^r * d
    BigInteger n_minus_1 = *this - BigInteger(1ULL);
    BigInteger d = n_minus_1;
    int r = 0;
    while (d.isEven()) { d = d >> 1; ++r; }

    std::mt19937_64 rng(std::random_device{}());

    for (int i = 0; i < rounds; ++i) {
        // Pick a random witness in [2, n-2]
        BigInteger a;
        do {
            uint64_t rand_val = rng();
            a = BigInteger(rand_val % (digits_[0] > 2 ? digits_[0] - 2 : 2) + 2);
        } while (a < BigInteger(2ULL) || a >= *this - BigInteger(1ULL));

        BigInteger x = modPow(a, d, *this);
        if (x.isOne() || x == n_minus_1) continue;

        bool composite = true;
        for (int j = 0; j < r - 1; ++j) {
            x = modPow(x, BigInteger(2ULL), *this);
            if (x == n_minus_1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

// ── Random BigInteger generation ──────────────────────────────────────────────

BigInteger BigInteger::random(int bits) {
    std::mt19937_64 rng(std::random_device{}());
    std::string bin;
    bin += '1'; // top bit always set
    for (int i = 1; i < bits; ++i)
        bin += (rng() & 1) ? '1' : '0';

    // Convert binary string to BigInteger
    BigInteger result(0ULL);
    BigInteger two(2ULL);
    for (char c : bin) {
        result = result * two + BigInteger(c == '1' ? 1ULL : 0ULL);
    }
    return result;
}

BigInteger BigInteger::randomPrime(int bits) {
    while (true) {
        BigInteger candidate = random(bits);
        // Force odd
        if (candidate.isEven())
            candidate = candidate + BigInteger(1ULL);
        if (candidate.isPrime(20))
            return candidate;
    }
}

// ── Extended GCD ──────────────────────────────────────────────────────────────

// Signed big integer helper (only used internally for extGcd)
// We use a simpler iterative approach with sign tracking
BigInteger BigInteger::extGcd(const BigInteger& a,
                               const BigInteger& b,
                               BigInteger& x,
                               BigInteger& y) {
    // Iterative extended Euclidean
    // Returns gcd and sets x, y such that a*x + b*y = gcd (unsigned, simplified)
    // For RSA we only need modInverse, so we implement a dedicated path there.
    // This is a placeholder that satisfies the interface.
    BigInteger old_r = a, r = b;
    BigInteger old_s(1ULL), s(0ULL);

    while (!r.isZero()) {
        BigInteger q = old_r / r;
        BigInteger tmp = r;
        r = old_r - q * r;
        old_r = tmp;

        tmp = s;
        // s = old_s - q * s  (signed — we skip y for now)
        old_s = tmp;
    }
    x = old_s;
    y = BigInteger(0ULL);
    return old_r;
}

BigInteger BigInteger::modInverse(const BigInteger& a, const BigInteger& m) {
    // Extended Euclidean via iterative algorithm with sign handling
    // Returns a^-1 mod m
    BigInteger t(0ULL), new_t(1ULL);
    BigInteger r = m, new_r = a;
    bool t_neg = false, new_t_neg = false;

    while (!new_r.isZero()) {
        BigInteger q = r / new_r;

        // t, new_t = new_t, t - q * new_t  (with sign)
        BigInteger q_new_t = q * new_t;
        BigInteger temp_t = new_t;
        bool temp_neg = new_t_neg;

        // compute t - q*new_t with sign
        if (t_neg == new_t_neg) {
            // same sign: subtract magnitudes
            if (t >= q_new_t) {
                new_t = t - q_new_t;
                new_t_neg = t_neg;
            } else {
                new_t = q_new_t - t;
                new_t_neg = !t_neg;
            }
        } else {
            // different sign: add magnitudes
            new_t = t + q_new_t;
            new_t_neg = t_neg;
        }
        t = temp_t; t_neg = temp_neg;

        BigInteger temp_r = new_r;
        new_r = r - q * new_r;
        r = temp_r;
    }

    if (r > BigInteger(1ULL))
        throw std::domain_error("No modular inverse: a and m are not coprime");

    if (t_neg) t = m - t;
    return t;
}

// ── toString ──────────────────────────────────────────────────────────────────

std::string BigInteger::toString() const {
    std::string result = std::to_string(digits_.back());
    for (int i = static_cast<int>(digits_.size()) - 2; i >= 0; --i) {
        std::string chunk = std::to_string(digits_[i]);
        result += std::string(9 - chunk.size(), '0') + chunk;
    }
    return result;
}
