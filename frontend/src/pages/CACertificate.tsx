import { useEffect, useState } from "react";
import { fetchCertTheory, issueCertificate } from "../api/certificate";
import type {
  CertTheoryResponse,
  IssuedCertificate,
  Packet,
} from "../types/certificate";
import "./CACertificate.css";

type Stage = "loading" | "theory" | "simulation" | "done" | "error";

interface Props {
  sessionId: string;
  onContinue: () => void;
}

export default function CACertificate({ sessionId, onContinue }: Props) {
  const [theory, setTheory] = useState<CertTheoryResponse | null>(null);
  const [stage, setStage] = useState<Stage>("loading");
  const [activeSection, setActiveSection] = useState<number>(0);
  const [error, setError] = useState<string | null>(null);

  // Simulation state
  const [packets, setPackets] = useState<Packet[]>([]);
  const [visiblePackets, setVisiblePackets] = useState<number>(0);
  const [certificate, setCertificate] = useState<IssuedCertificate | null>(null);

  useEffect(() => {
    fetchCertTheory()
      .then((data) => {
        setTheory(data);
        setStage("theory");
      })
      .catch((e: Error) => {
        setError(e.message);
        setStage("error");
      });
  }, []);

  const runSimulation = async () => {
    setStage("simulation");
    setVisiblePackets(0);
    try {
      const result = await issueCertificate(sessionId, "Alice");
      setPackets(result.packets);
      setCertificate(result.certificate);

      // Reveal packets one by one
      for (let i = 1; i <= result.packets.length; i++) {
        await new Promise((r) => setTimeout(r, 800));
        setVisiblePackets(i);
      }
      await new Promise((r) => setTimeout(r, 400));
      setStage("done");
    } catch (e) {
      setError((e as Error).message);
      setStage("error");
    }
  };

  if (stage === "loading") {
    return <div className="ca-loading">Loading certificate theory…</div>;
  }

  if (stage === "error") {
    return <div className="ca-error">Error: {error}</div>;
  }

  return (
    <main className="ca-cert">
      <h1>Certificate Authority &amp; Digital Certificates</h1>

      {/* ── Theory ── */}
      {theory && (
        <section className="ca-theory" aria-label="Certificate theory">
          <p className="ca-intro">
            Before the simulation, read through how digital certificates work.
          </p>

          <ol className="ca-section-list">
            {theory.sections.map((s) => (
              <li
                key={s.index}
                className={`ca-section-item ${activeSection === s.index ? "ca-section-active" : ""}`}
                onClick={() =>
                  setActiveSection(activeSection === s.index ? 0 : s.index)
                }
              >
                <span className="ca-section-num">{s.index}</span>
                <div className="ca-section-body">
                  <strong>{s.title}</strong>
                  {activeSection === s.index && (
                    <p className="ca-section-text">{s.body}</p>
                  )}
                </div>
              </li>
            ))}
          </ol>

          {/* Mock certificate breakdown */}
          <div className="ca-mock-cert">
            <h2>Example Certificate Fields</h2>
            <table className="ca-field-table">
              <thead>
                <tr>
                  <th>Field</th>
                  <th>Example Value</th>
                  <th>Meaning</th>
                </tr>
              </thead>
              <tbody>
                {Object.entries(theory.mock_certificate.field_explanations).map(
                  ([field, explanation]) => {
                    const mock = theory.mock_certificate as unknown as Record<string, unknown>;
                    const val = mock[field];
                    const display =
                      val && typeof val === "object"
                        ? JSON.stringify(val)
                        : String(val ?? "—");
                    return (
                      <tr key={field}>
                        <td>
                          <code>{field}</code>
                        </td>
                        <td>
                          <code className="ca-mock-val">{display}</code>
                        </td>
                        <td>{explanation}</td>
                      </tr>
                    );
                  }
                )}
              </tbody>
            </table>
          </div>
        </section>
      )}

      {/* ── Gate: start simulation ── */}
      {(stage === "theory") && (
        <div className="ca-gate">
          <button className="ca-btn" onClick={runSimulation}>
            Run Certificate Simulation →
          </button>
        </div>
      )}

      {/* ── Packet animation ── */}
      {(stage === "simulation" || stage === "done") && (
        <section className="ca-simulation" aria-label="Certificate issuance simulation">
          <h2>Certificate Issuance — Packet Exchange</h2>
          <div className="ca-packets">
            {packets.slice(0, visiblePackets).map((pkt) => (
              <div
                key={pkt.index}
                className={`ca-packet ca-packet-${pkt.index}`}
                aria-label={`Packet ${pkt.index}: ${pkt.type}`}
              >
                <div className="ca-packet-header">
                  <span className="ca-packet-badge">{pkt.index}</span>
                  <span className="ca-packet-route">
                    {pkt.from} → {pkt.to}
                  </span>
                  <span className="ca-packet-type">{pkt.type}</span>
                </div>
                <p className="ca-packet-desc">{pkt.description}</p>
              </div>
            ))}

            {stage === "simulation" && visiblePackets < 2 && (
              <div className="ca-packet-loading" aria-label="Processing">
                <span className="ca-spinner">⟳</span> Exchanging packets…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Certificate display ── */}
      {stage === "done" && certificate && (
        <section className="ca-issued-cert" aria-label="Issued certificate">
          <h2>Issued Certificate</h2>
          <div className="ca-cert-card">
            <div className="ca-cert-row">
              <span className="ca-cert-label">Subject</span>
              <span className="ca-cert-value">{certificate.subject}</span>
            </div>
            <div className="ca-cert-row">
              <span className="ca-cert-label">Issuer</span>
              <span className="ca-cert-value">{certificate.issuer}</span>
            </div>
            <div className="ca-cert-row">
              <span className="ca-cert-label">Serial</span>
              <code className="ca-cert-value">{certificate.serial}</code>
            </div>
            <div className="ca-cert-row">
              <span className="ca-cert-label">Valid From</span>
              <span className="ca-cert-value">{certificate.valid_from}</span>
            </div>
            <div className="ca-cert-row">
              <span className="ca-cert-label">Valid To</span>
              <span className="ca-cert-value">{certificate.valid_to}</span>
            </div>
            <div className="ca-cert-row">
              <span className="ca-cert-label">Public Key (e)</span>
              <code className="ca-cert-value">{certificate.public_key.e}</code>
            </div>
            <div className="ca-cert-row ca-cert-row-long">
              <span className="ca-cert-label">Public Key (n)</span>
              <code
                className="ca-cert-value ca-cert-truncate"
                title={certificate.public_key.n}
              >
                {certificate.public_key.n.slice(0, 32)}…
              </code>
            </div>
            <div className="ca-cert-row ca-cert-row-long">
              <span className="ca-cert-label">Cert Hash</span>
              <code className="ca-cert-value">{certificate.cert_hash}</code>
            </div>
            <div className="ca-cert-row ca-cert-row-long">
              <span className="ca-cert-label">CA Signature</span>
              <code
                className="ca-cert-value ca-cert-truncate"
                title={certificate.ca_signature}
              >
                {certificate.ca_signature.slice(0, 32)}…
              </code>
            </div>

            <div className="ca-cert-valid-banner" aria-label="Certificate valid">
              ✓ Certificate issued and signed by CA
            </div>
          </div>

          <div className="ca-gate">
            <button className="ca-btn" onClick={onContinue}>
              Continue →
            </button>
          </div>
        </section>
      )}
    </main>
  );
}
