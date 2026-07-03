#include "BigInteger.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <sstream>

// Internal representation: little-endian base-2^32 limbs (uint32_t).
// This makes bit-level operations (>>, <<, isEven, getBit) fast — critical
// for Miller-Rabin and modPow performance on large numbers.

static constexpr uint64_t LIMB_BASE = 0x100000000ULL; // 2^32

// ── Constructors ──────────────────────────────────────────────────────────────

BigInteger::BigInteger() : digits_{0} {}

BigInteger::BigInteger(uint64_t value) {
    if (value == 0) { digits_.push_back(0); return; }
    while (value > 0) {
        digits_.push_back(static_cast<uint32_t>(value & 0xFFFFFFFFULL));
        value >>= 32;
    }
}

BigInteger::BigInteger(const std::string& decimal) {
    // Parse decimal string by repeated multiply-add
    digits_.push_back(0);
    for (char c : decimal) {
        if (c < '0' || c > '9') continue;
        // *this = *this * 10 + digit
        uint64_t carry = static_cast<uint64_t>(c - '0');
        for (auto& limb : digits_) {
            uint64_t cur = static_cast<uint64_t>(limb) * 10 + carry;
            limb = static_cast<uint32_t>(cur & 0xFFFFFFFFULL);
            carry = cur >> 32;
        }
        if (carry) digits_.push_back(static_cast<uint32_t>(carry));
    }
    trim();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void BigInteger::trim() {
    while (digits_.size() > 1 && digits_.back() == 0)
        digits_.pop_back();
}

BigInteger BigInteger::fromDigits(std::vector<uint32_t> d) {
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
    return false;
}

bool BigInteger::operator<=(const BigInteger& rhs) const { return !(rhs < *this); }
bool BigInteger::operator>(const BigInteger& rhs) const  { return rhs < *this; }
bool BigInteger::operator>=(const BigInteger& rhs) const { return !(*this < rhs); }

// ── Addition ──────────────────────────────────────────────────────────────────

BigInteger BigInteger::operator+(const BigInteger& rhs) const {
    std::vector<uint32_t> res;
    uint64_t carry = 0;
    size_t n = std::max(digits_.size(), rhs.digits_.size());
    for (size_t i = 0; i < n || carry; ++i) {
        uint64_t sum = carry;
        if (i < digits_.size())     sum += digits_[i];
        if (i < rhs.digits_.size()) sum += rhs.digits_[i];
        res.push_back(static_cast<uint32_t>(sum & 0xFFFFFFFFULL));
        carry = sum >> 32;
    }
    return fromDigits(std::move(res));
}

// ── Subtraction ───────────────────────────────────────────────────────────────

BigInteger BigInteger::operator-(const BigInteger& rhs) const {
    std::vector<uint32_t> res;
    int64_t borrow = 0;
    for (size_t i = 0; i < digits_.size(); ++i) {
        int64_t diff = static_cast<int64_t>(digits_[i]) - borrow
                     - (i < rhs.digits_.size() ? static_cast<int64_t>(rhs.digits_[i]) : 0LL);
        if (diff < 0) { diff += static_cast<int64_t>(LIMB_BASE); borrow = 1; }
        else borrow = 0;
        res.push_back(static_cast<uint32_t>(diff));
    }
    return fromDigits(std::move(res));
}

// ── Multiplication ────────────────────────────────────────────────────────────

BigInteger BigInteger::operator*(const BigInteger& rhs) const {
    std::vector<uint32_t> res(digits_.size() + rhs.digits_.size(), 0);
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < rhs.digits_.size() || carry; ++j) {
            uint64_t cur = static_cast<uint64_t>(res[i + j]) + carry;
            if (j < rhs.digits_.size())
                cur += static_cast<uint64_t>(digits_[i]) * rhs.digits_[j];
            res[i + j] = static_cast<uint32_t>(cur & 0xFFFFFFFFULL);
            carry = cur >> 32;
        }
    }
    return fromDigits(std::move(res));
}

// ── Division & Modulo (binary long division) ──────────────────────────────────

// Shift left by 1 bit in-place
static void shl1(std::vector<uint32_t>& v) {
    uint32_t carry = 0;
    for (auto& limb : v) {
        uint32_t next = limb >> 31;
        limb = (limb << 1) | carry;
        carry = next;
    }
    if (carry) v.push_back(carry);
}

// Shift right by 1 bit in-place
static void shr1(std::vector<uint32_t>& v) {
    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i) {
        if (i > 0) v[i - 1] |= (v[i] & 1) << 31;
        v[i] >>= 1;
    }
    while (v.size() > 1 && v.back() == 0) v.pop_back();
}

static bool limb_lt(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    if (a.size() != b.size()) return a.size() < b.size();
    for (int i = static_cast<int>(a.size()) - 1; i >= 0; --i)
        if (a[i] != b[i]) return a[i] < b[i];
    return false;
}

static bool limb_lte(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    return !limb_lt(b, a);
}

// In-place subtract: a -= b (assumes a >= b)
static void limb_sub_inplace(std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    int64_t borrow = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        int64_t diff = static_cast<int64_t>(a[i]) - borrow
                     - (i < b.size() ? static_cast<int64_t>(b[i]) : 0LL);
        if (diff < 0) { diff += static_cast<int64_t>(LIMB_BASE); borrow = 1; }
        else borrow = 0;
        a[i] = static_cast<uint32_t>(diff);
    }
    while (a.size() > 1 && a.back() == 0) a.pop_back();
}

std::pair<BigInteger, BigInteger> BigInteger::divmod(const BigInteger& a,
                                                      const BigInteger& b) {
    if (b.isZero()) throw std::domain_error("Division by zero");
    if (a < b) return {BigInteger(0ULL), a};

    int n = a.bitLength();
    // quotient accumulator
    std::vector<uint32_t> q((a.digits_.size()), 0);
    // remainder: start with 0, build bit by bit from MSB of a
    std::vector<uint32_t> r = {0};

    for (int i = n - 1; i >= 0; --i) {
        // r = r * 2 + bit i of a
        shl1(r);
        // set bit i of a
        uint32_t bit = (a.digits_[i / 32] >> (i % 32)) & 1;
        r[0] |= bit;

        // if r >= b: r -= b, set bit i of quotient
        if (limb_lte(b.digits_, r)) {
            limb_sub_inplace(r, b.digits_);
            // set bit i in q
            if (static_cast<size_t>(i / 32) >= q.size())
                q.resize(i / 32 + 1, 0);
            q[i / 32] |= (1u << (i % 32));
        }
    }

    return {fromDigits(std::move(q)), fromDigits(std::move(r))};
}

BigInteger BigInteger::operator/(const BigInteger& rhs) const {
    return divmod(*this, rhs).first;
}

BigInteger BigInteger::operator%(const BigInteger& mod) const {
    if (mod.isZero()) throw std::domain_error("Modulo by zero");
    if (*this < mod) return *this;
    return divmod(*this, mod).second;
}

// ── Bit operations ────────────────────────────────────────────────────────────

bool BigInteger::isZero() const { return digits_.size() == 1 && digits_[0] == 0; }
bool BigInteger::isOne()  const { return digits_.size() == 1 && digits_[0] == 1; }
bool BigInteger::isEven() const { return (digits_[0] & 1) == 0; }

int BigInteger::bitLength() const {
    if (isZero()) return 0;
    int top_bits = 0;
    uint32_t top = digits_.back();
    while (top > 0) { top >>= 1; ++top_bits; }
    return static_cast<int>((digits_.size() - 1)) * 32 + top_bits;
}

bool BigInteger::getBit(int pos) const {
    int limb  = pos / 32;
    int shift = pos % 32;
    if (static_cast<size_t>(limb) >= digits_.size()) return false;
    return (digits_[limb] >> shift) & 1;
}

// Right-shift by `shift` bits — O(n) limb operations
BigInteger BigInteger::operator>>(int shift) const {
    if (shift <= 0 || isZero()) return *this;
    int limb_shift = shift / 32;
    int bit_shift  = shift % 32;

    if (static_cast<size_t>(limb_shift) >= digits_.size())
        return BigInteger(0ULL);

    std::vector<uint32_t> res;
    for (size_t i = static_cast<size_t>(limb_shift); i < digits_.size(); ++i) {
        uint32_t lo = digits_[i] >> bit_shift;
        uint32_t hi = (bit_shift > 0 && i + 1 < digits_.size())
                      ? (digits_[i + 1] << (32 - bit_shift))
                      : 0;
        res.push_back(lo | hi);
    }
    return fromDigits(std::move(res));
}

// Left-shift by `shift` bits — O(n)
BigInteger BigInteger::operator<<(int shift) const {
    if (shift <= 0 || isZero()) return *this;
    int limb_shift = shift / 32;
    int bit_shift  = shift % 32;

    std::vector<uint32_t> res(digits_.size() + static_cast<size_t>(limb_shift) + 1, 0);
    for (size_t i = 0; i < digits_.size(); ++i) {
        res[i + static_cast<size_t>(limb_shift)] |= digits_[i] << bit_shift;
        if (bit_shift > 0 && i + static_cast<size_t>(limb_shift) + 1 < res.size())
            res[i + static_cast<size_t>(limb_shift) + 1] |= digits_[i] >> (32 - bit_shift);
    }
    return fromDigits(std::move(res));
}

// ── Modular exponentiation (square-and-multiply) ──────────────────────────────

BigInteger BigInteger::modPow(const BigInteger& base,
                              const BigInteger& exp,
                              const BigInteger& mod) {
    if (mod.isOne()) return BigInteger(0ULL);
    BigInteger result(1ULL);
    BigInteger b = base % mod;
    int bits = exp.bitLength();

    for (int i = 0; i < bits; ++i) {
        if (exp.getBit(i)) result = result * b % mod;
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
    BigInteger n_minus_2 = *this - BigInteger(2ULL);

    for (int i = 0; i < rounds; ++i) {
        // Random witness in [2, n-2]: generate random number < n-2, then +2
        int bl = n_minus_2.bitLength();
        BigInteger a(0ULL);
        do {
            // Build a random number with `bl` bits
            std::vector<uint32_t> limbs((bl + 31) / 32, 0);
            for (auto& limb : limbs) limb = static_cast<uint32_t>(rng());
            // Mask top limb to exact bit length
            int top_bits = bl % 32;
            if (top_bits) limbs.back() &= (1u << top_bits) - 1;
            a = fromDigits(std::move(limbs));
        } while (a < BigInteger(2ULL) || a > n_minus_2);

        BigInteger x = modPow(a, d, *this);
        if (x.isOne() || x == n_minus_1) continue;

        bool composite = true;
        for (int j = 0; j < r - 1; ++j) {
            x = x * x % *this;
            if (x == n_minus_1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

// ── Random BigInteger generation ──────────────────────────────────────────────

BigInteger BigInteger::random(int bits) {
    std::mt19937_64 rng(std::random_device{}());
    int limbs = (bits + 31) / 32;
    std::vector<uint32_t> v(static_cast<size_t>(limbs), 0);
    for (auto& limb : v)
        limb = static_cast<uint32_t>(rng());

    // Determine how many bits occupy the top limb
    int top_bits = bits % 32; // 0 means the top limb is a full 32-bit limb
    if (top_bits == 0) {
        // All 32 bits of the top limb are valid — just set the MSB
        v.back() |= 0x80000000u;
    } else {
        // Mask off bits above `top_bits`, then set the MSB within range
        uint32_t mask = (1u << top_bits) - 1u;
        v.back() &= mask;
        v.back() |= (1u << (top_bits - 1)); // set MSB
    }
    return fromDigits(std::move(v));
}

BigInteger BigInteger::randomPrime(int bits) {
    while (true) {
        BigInteger candidate = random(bits);
        if (candidate.isEven())
            candidate = candidate + BigInteger(1ULL);
        if (candidate.isPrime(20))
            return candidate;
    }
}

// ── Extended GCD ──────────────────────────────────────────────────────────────

BigInteger BigInteger::extGcd(const BigInteger& a,
                               const BigInteger& b,
                               BigInteger& x,
                               BigInteger& y) {
    BigInteger old_r = a, r = b;
    BigInteger old_s(1ULL), s(0ULL);

    while (!r.isZero()) {
        BigInteger q = old_r / r;
        BigInteger tmp = r;
        r = old_r - q * r;
        old_r = tmp;
        tmp = s;
        old_s = tmp;
    }
    x = old_s;
    y = BigInteger(0ULL);
    return old_r;
}

BigInteger BigInteger::modInverse(const BigInteger& a, const BigInteger& m) {
    BigInteger t(0ULL), new_t(1ULL);
    BigInteger r = m, new_r = a;
    bool t_neg = false, new_t_neg = false;

    while (!new_r.isZero()) {
        BigInteger q = r / new_r;

        BigInteger q_new_t = q * new_t;
        BigInteger temp_t = new_t;
        bool temp_neg = new_t_neg;

        if (t_neg == new_t_neg) {
            if (t >= q_new_t) {
                new_t = t - q_new_t;
                new_t_neg = t_neg;
            } else {
                new_t = q_new_t - t;
                new_t_neg = !t_neg;
            }
        } else {
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
    if (isZero()) return "0";
    // Repeated division by 10 to extract decimal digits
    BigInteger n = *this;
    BigInteger ten(10ULL);
    std::string result;
    while (!n.isZero()) {
        auto [q, rem] = divmod(n, ten);
        result += static_cast<char>('0' + rem.digits_[0]);
        n = q;
    }
    std::reverse(result.begin(), result.end());
    return result;
}
