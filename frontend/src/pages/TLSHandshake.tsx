import { useState } from "react";
import { runTLSHandshake } from "../api/tls";
import type { TLSHandshakeResponse, TLSPacket } from "../types/tls";
import "./TLSHandshake.css";

type Stage = "choose" | "running" | "done" | "error";

const TYPE_ICONS: Record<string, string> = {
  ClientHello:        "👋",
  ServerHello:        "🤝",
  CertificateVerify:  "🔍",
  ClientKeyExchange:  "🔐",
  "Finished (Server)":"✅",
  "Finished (Client)":"✅",
  Alert:              "🚨",
};

const TLS_STEPS = [
  { num: 1, label: "ClientHello",       who: "Bob → Alice",  desc: "Bob announces supported TLS versions, cipher suites, and a random nonce." },
  { num: 2, label: "ServerHello",       who: "Alice → Bob",  desc: "Alice picks the cipher, sends her certificate and a server nonce." },
  { num: 3, label: "CertVerify",        who: "Bob verifies",  desc: "Bob checks the CA signature on Alice's certificate." },
  { num: 4, label: "KeyExchange",       who: "Bob → Alice",  desc: "Bob generates a session key and encrypts it with Alice's public key." },
  { num: 5, label: "Finished (Server)", who: "Alice → Bob",  desc: "Alice decrypts the key and sends HMAC of the transcript to prove she holds the right key." },
  { num: 6, label: "Finished (Client)", who: "Bob → Alice",  desc: "Bob verifies the HMAC. Handshake complete — secure channel established." },
];

interface Props {
  sessionId: string;
  onContinue: () => void;
}

export default function TLSHandshake({ sessionId, onContinue }: Props) {
  const [stage, setStage] = useState<Stage>("choose");
  const [mode, setMode] = useState<"normal" | "fake_alice">("normal");
  const [result, setResult] = useState<TLSHandshakeResponse | null>(null);
  const [visiblePackets, setVisiblePackets] = useState(0);
  const [error, setError] = useState<string | null>(null);

  const run = async () => {
    setStage("running");
    setVisiblePackets(0);
    setError(null);
    try {
      const res = await runTLSHandshake(sessionId, mode === "fake_alice");
      setResult(res);
      for (let i = 1; i <= res.packets.length; i++) {
        await new Promise((r) => setTimeout(r, 700));
        setVisiblePackets(i);
      }
      setStage("done");
    } catch (e) {
      setError((e as Error).message);
      setStage("error");
    }
  };

  return (
    <main className="tls-page">
      <h1 className="tls-h1">TLS Handshake Simulation</h1>
      <p className="tls-intro">
        TLS (Transport Layer Security) uses RSA to securely exchange a session
        key before any data is sent. Below is a simplified TLS 1.3-like
        handshake — built from scratch without any TLS library.
      </p>

      {/* ── Protocol overview ── */}
      <section className="tls-overview">
        <h2 className="tls-h2">Handshake Protocol</h2>
        <div className="tls-steps-overview">
          {TLS_STEPS.map((s) => (
            <div key={s.num} className="tls-ov-row">
              <span className="tls-ov-num">{s.num}</span>
              <span className="tls-ov-label">{s.label}</span>
              <span className="tls-ov-who">{s.who}</span>
              <span className="tls-ov-desc">{s.desc}</span>
            </div>
          ))}
        </div>
      </section>

      {/* ── Mode selector ── */}
      <section className="tls-mode-section">
        <h2 className="tls-h2">Choose Mode</h2>
        <div className="tls-modes">
          <button
            className={`tls-mode-btn ${mode === "normal" ? "tls-mode-active" : ""}`}
            onClick={() => setMode("normal")}
          >
            <span className="tls-mode-icon">🔒</span>
            <div>
              <strong>Normal Handshake</strong>
              <p>Bob connects to Alice. Certificate is valid. Session key exchanged securely. HMAC verified.</p>
            </div>
          </button>
          <button
            className={`tls-mode-btn ${mode === "fake_alice" ? "tls-mode-active-danger" : ""}`}
            onClick={() => setMode("fake_alice")}
          >
            <span className="tls-mode-icon">🎭</span>
            <div>
              <strong>Fake-Alice Injection</strong>
              <p>Fake-Alice intercepts the handshake with a forged certificate. The TLS layer detects the attack and aborts.</p>
            </div>
          </button>
        </div>

        {stage === "choose" && (
          <div className="tls-gate">
            <button className={`tls-btn ${mode === "fake_alice" ? "tls-btn-danger" : "tls-btn-primary"}`} onClick={run}>
              Run Handshake →
            </button>
          </div>
        )}
      </section>

      {/* ── Packet animation ── */}
      {(stage === "running" || stage === "done") && result && (
        <section className="tls-packets-section">
          <h2 className="tls-h2">Handshake Packets</h2>
          <div className="tls-lane">
            <div className="tls-lane-label">Bob (Client)</div>
            <div className="tls-lane-label">Alice (Server)</div>
          </div>
          <div className="tls-packets">
            {result.packets.slice(0, visiblePackets).map((pkt) => (
              <PacketRow key={pkt.index} packet={pkt} />
            ))}
            {stage === "running" && visiblePackets < result.packets.length && (
              <div className="tls-loading">
                <span className="tls-spinner">⟳</span> Sending packet {visiblePackets + 1}…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Result ── */}
      {stage === "done" && result && (
        <>
          <div className={`tls-result ${result.success ? "tls-result-ok" : "tls-result-fail"}`} role="alert">
            <span className="tls-result-icon">{result.success ? "🔒" : "🚨"}</span>
            <div>
              <strong>
                {result.success
                  ? "Secure Channel Established"
                  : "Handshake Failed — Attack Detected"}
              </strong>
              <p>
                {result.success
                  ? "HMAC verified · Session key exchanged · All 6 steps completed successfully"
                  : result.failure_reason}
              </p>
            </div>
          </div>

          <div className="tls-gate">
            <button className="tls-btn" onClick={() => { setStage("choose"); setResult(null); setVisiblePackets(0); }}>
              ← Run again
            </button>
            <button className="tls-btn tls-btn-primary" onClick={onContinue}>
              Continue →
            </button>
          </div>
        </>
      )}

      {stage === "error" && <div className="tls-error">{error}</div>}
    </main>
  );
}

function PacketRow({ packet }: { packet: TLSPacket }) {
  const fromBob = packet.from === "Bob";
  const isAlert = packet.type === "Alert";
  const icon = TYPE_ICONS[packet.type] ?? "📦";

  return (
    <div className={`tls-pkt ${fromBob ? "tls-pkt-right" : "tls-pkt-left"} ${isAlert ? "tls-pkt-alert" : ""} ${!packet.ok ? "tls-pkt-fail" : ""}`}>
      <div className="tls-pkt-connector">
        <div className="tls-pkt-dot" />
        <div className={`tls-pkt-line ${fromBob ? "tls-line-right" : "tls-line-left"}`} />
      </div>
      <div className="tls-pkt-card">
        <div className="tls-pkt-hdr">
          <span className="tls-pkt-icon">{icon}</span>
          <span className="tls-pkt-index">{packet.index}</span>
          <span className="tls-pkt-type">{packet.type}</span>
          <span className="tls-pkt-route">{packet.from} → {packet.to}</span>
        </div>
        <p className="tls-pkt-desc">{packet.description}</p>
        <PayloadFields payload={packet.payload} />
      </div>
    </div>
  );
}

function PayloadFields({ payload }: { payload: Record<string, unknown> }) {
  const entries = Object.entries(payload).filter(([k]) => k !== "extensions");
  return (
    <div className="tls-payload">
      {entries.map(([k, v]) => (
        <div key={k} className="tls-payload-row">
          <span className="tls-payload-key">{k}</span>
          <code className="tls-payload-val">
            {typeof v === "object" ? JSON.stringify(v) : String(v)}
          </code>
        </div>
      ))}
    </div>
  );
}
