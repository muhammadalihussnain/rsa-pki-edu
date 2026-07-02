import { useEffect, useState } from "react";
import { fetchRSATheory } from "../api/education";
import type { RSATheoryResponse, TheoryStep } from "../types/education";
import "./RSATheory.css";

interface Props {
  onContinue: () => void;
}

const SECURITY_COLOR: Record<string, string> = {
  High: "sec-high",
  Medium: "sec-medium",
  Low: "sec-low",
};

export default function RSATheory({ onContinue }: Props) {
  const [data, setData] = useState<RSATheoryResponse | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [activeStep, setActiveStep] = useState<number | null>(null);

  useEffect(() => {
    fetchRSATheory()
      .then(setData)
      .catch((e: Error) => setError(e.message));
  }, []);

  if (error) return <div className="rt-state rt-error">Error: {error}</div>;
  if (!data) return <div className="rt-state">Loading RSA theory…</div>;

  const toggle = (i: number) => setActiveStep(activeStep === i ? null : i);

  return (
    <main className="rt-page">

      {/* ── 1. What is RSA ── */}
      <section className="rt-section">
        <h1 className="rt-h1">{data.what_is_rsa.title}</h1>
        <p className="rt-body">{data.what_is_rsa.body}</p>
        <ul className="rt-uses">
          {data.what_is_rsa.uses.map((u) => (
            <li key={u} className="rt-use-item">{u}</li>
          ))}
        </ul>
      </section>

      {/* ── 2. RSA Flavours comparison ── */}
      <section className="rt-section">
        <h2 className="rt-h2">RSA Variants</h2>
        <p className="rt-body">
          RSA is not one algorithm — it comes in several padding schemes. Each
          serves a different purpose and has different security properties.
        </p>
        <div className="rt-table-wrap">
          <table className="rt-table">
            <thead>
              <tr>
                <th>Variant</th>
                <th>Full Name</th>
                <th>Use</th>
                <th>Security</th>
                <th>Notes</th>
              </tr>
            </thead>
            <tbody>
              {data.flavours.map((f) => (
                <tr key={f.name}>
                  <td><code className="rt-code">{f.name}</code></td>
                  <td className="rt-muted">{f.full_name}</td>
                  <td>{f.use}</td>
                  <td>
                    <span className={`rt-badge ${SECURITY_COLOR[f.security] ?? ""}`}>
                      {f.security}
                    </span>
                  </td>
                  <td className="rt-muted">{f.note}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <p className="rt-callout">
          This simulator uses <strong>Raw (Textbook) RSA</strong> so you can
          follow the math step by step. Never use raw RSA in real software.
        </p>
      </section>

      {/* ── 3. Key generation steps ── */}
      <section className="rt-section">
        <h2 className="rt-h2">Key Generation Process</h2>
        <p className="rt-body">
          Click each step to expand the explanation and see the required
          condition.
        </p>
        <ol className="rt-steps">
          {data.steps.map((step: TheoryStep) => (
            <li
              key={step.index}
              className={`rt-step ${activeStep === step.index ? "rt-step-open" : ""}`}
              onClick={() => toggle(step.index)}
              aria-expanded={activeStep === step.index}
            >
              <div className="rt-step-header">
                <span className="rt-step-num">{step.index}</span>
                <span className="rt-step-title">{step.title}</span>
                <span className="rt-step-chevron">
                  {activeStep === step.index ? "▲" : "▼"}
                </span>
              </div>
              {activeStep === step.index && (
                <div className="rt-step-body">
                  <p className="rt-step-explain">{step.explanation}</p>
                  <div className="rt-condition">
                    <span className="rt-condition-label">Condition</span>
                    <code className="rt-condition-val">{step.condition}</code>
                  </div>
                </div>
              )}
            </li>
          ))}
        </ol>
      </section>

      {/* ── 4. Worked example ── */}
      <section className="rt-section rt-example-section">
        <h2 className="rt-h2">Worked Example</h2>
        <p className="rt-body">
          Using small numbers (<strong>p = {data.example.p}</strong>,{" "}
          <strong>q = {data.example.q}</strong>) so every calculation is easy
          to verify by hand. Real RSA uses primes with 1024+ digits.
        </p>

        <div className="rt-example-steps">
          {data.example.steps.map((s, i) => (
            <div key={i} className="rt-ex-row">
              <div className="rt-ex-num">{i + 1}</div>
              <div className="rt-ex-content">
                <code className="rt-ex-label">{s.label}</code>
                <div className="rt-ex-meta">
                  <span className="rt-ex-condition">{s.condition}</span>
                  <span className="rt-ex-note">{s.note}</span>
                </div>
              </div>
            </div>
          ))}
        </div>

        <div className="rt-keys">
          <div className="rt-key-card rt-public">
            <div className="rt-key-tag">Public Key</div>
            <div className="rt-key-val">e = {data.example.public_key.e}</div>
            <div className="rt-key-val">n = {data.example.public_key.n}</div>
            <div className="rt-key-note">Share this openly</div>
          </div>
          <div className="rt-key-card rt-private">
            <div className="rt-key-tag">Private Key</div>
            <div className="rt-key-val">d = {data.example.private_key.d}</div>
            <div className="rt-key-val">n = {data.example.private_key.n}</div>
            <div className="rt-key-note">Keep this secret</div>
          </div>
        </div>

        <div className="rt-verify">
          <span className="rt-verify-label">Verify</span>
          {data.example.verify}
        </div>
      </section>

      {/* ── Gate ── */}
      <div className="rt-gate">
        <button className="rt-continue" onClick={onContinue}>
          I understand, continue →
        </button>
      </div>
    </main>
  );
}
