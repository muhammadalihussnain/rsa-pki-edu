# RSA PKI Edu

[![Backend CI](https://github.com/muhammadalihussnain/rsa-pki-edu/actions/workflows/backend-ci.yml/badge.svg)](https://github.com/muhammadalihussnain/rsa-pki-edu/actions/workflows/backend-ci.yml)
[![Frontend CI](https://github.com/muhammadalihussnain/rsa-pki-edu/actions/workflows/frontend-ci.yml/badge.svg)](https://github.com/muhammadalihussnain/rsa-pki-edu/actions/workflows/frontend-ci.yml)

An interactive, step-by-step educational simulator for RSA public-key cryptography, PKI, and TLS — built entirely from scratch with no pre-built crypto libraries.

---

## What it teaches

| Screen | Concept |
|--------|---------|
| RSA Theory | Key generation math, variants (OAEP, PSS, Raw), worked example with conditions |
| Key Generation | Generating RSA key pairs for Alice, Bob, CA, and Fake-Alice |
| CA Certificate | How a Certificate Authority issues and signs X.509-style certificates |
| Sign Document | SHA-256 (custom) + RSA signing of a document hash |
| Bob Verifies | 5-step certificate + signature verification using only public keys |
| Attacker Scenario | Forged certificate and tampered document — both caught by PKI |
| TLS Handshake | Simplified TLS 1.3-like handshake with custom HMAC — attack detection included |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Browser (React)                      │
│  RSATheory → KeyGen → CACert → Sign → BobVerify            │
│           → AttackerScenario → TLSHandshake                 │
└──────────────────────┬──────────────────────────────────────┘
                       │ HTTP (Vite proxy → :8081)
┌──────────────────────▼──────────────────────────────────────┐
│                  C++ Backend (cpp-httplib)                   │
│                                                             │
│  BigInteger ──► RSACrypto ──► Sessions                      │
│       │                          │                          │
│       ▼                          ▼                          │
│    SHA256 ──► HMAC          CertificateAuthority            │
│       │                          │                          │
│       ▼                          ▼                          │
│  DocumentHandler          BobHandler / AttackerHandler      │
│                                  │                          │
│                            TLSSession (handshake sim)       │
└─────────────────────────────────────────────────────────────┘
```

No pre-built crypto libraries are used. Every algorithm — RSA, SHA-256, HMAC, Miller-Rabin primality — is implemented from scratch.

---

## Setup

### Prerequisites

| Tool | Version |
|------|---------|
| g++ | ≥ 11 |
| cmake | ≥ 3.14 |
| Node.js | ≥ 20 |
| npm | ≥ 10 |

### Backend

```bash
# From repo root
cmake -S backend -B backend/build -DENABLE_COVERAGE=OFF
cmake --build backend/build --parallel $(nproc)
./backend/build/rsa_server
# Server listens on http://localhost:8081
```

### Frontend

```bash
cd frontend
npm install
npm run dev
# App available at http://localhost:5173
```

The Vite dev server proxies `/api/*` requests to `:8081` automatically (configured in `vite.config.ts`).

### Run tests

```bash
ctest --test-dir backend/build --output-on-failure
```

---

## API Endpoints

| Method | Path | Phase | Description |
|--------|------|-------|-------------|
| GET | `/api/health` | 1 | Health check |
| GET | `/api/education/rsa-theory` | 2 | RSA theory JSON |
| POST | `/api/session/init` | 3 | Generate key pairs for all actors |
| GET | `/api/education/certificate-theory` | 4 | Certificate theory JSON |
| POST | `/api/ca/issue-certificate` | 4 | Issue CA-signed certificate |
| POST | `/api/alice/upload-document` | 5 | Upload doc, returns SHA-256 hash |
| POST | `/api/alice/sign-document` | 5 | Sign hash with Alice's private key |
| GET | `/api/education/certificate-verification` | 6 | Cert verification theory |
| GET | `/api/education/signature-verification` | 6 | Signature verification theory |
| POST | `/api/bob/verify` | 6 | 5-step verification log |
| POST | `/api/fake-alice/attempt` | 7 | Attacker scenario (forged cert / tampered doc) |
| POST | `/api/tls/handshake` | 8 | TLS 1.3-like handshake simulation |

---

## Security properties verified

- Private keys never appear in any API response
- SHA-256 output matches FIPS 180-4 test vectors
- HMAC follows RFC 2104
- Certificate forgery always fails Step 1 (CA signature check)
- Document tampering always fails Step 5 (hash mismatch)
- TLS injection detected via HMAC mismatch in Finished step

---

## Project structure

```
rsa-pki-edu/
├── backend/
│   ├── include/          # Headers: BigInteger, RSA, Hash, HMAC, TLS, ...
│   ├── src/              # Implementations
│   └── tests/            # Google Test suites (100 tests)
├── frontend/
│   └── src/
│       ├── api/          # Fetch wrappers per module
│       ├── pages/        # One .tsx + .css per screen
│       └── types/        # TypeScript interfaces
└── .github/workflows/    # CI: backend-ci, frontend-ci, coverage
```
