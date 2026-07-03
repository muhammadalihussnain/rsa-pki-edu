import { useState } from "react";
import { fakeAliceAttempt, bobVerifyAttack } from "../api/attacker";
import type { AttackerResponse, AttackScenario } from "../types/attacker";
import type { VerifyResponse, VerifyStep } from "../types/verification";
import "./AttackerScenario.css";

type Stage = "choose" | "attacking" | "verifying" | "done" | "error";

interface Props {
  sessionId: string;
  originalDocument: string;
  onContinue: () => void;
}

export default function AttackerScenario({ sessionId, originalDocument, onContinue }: Props) {
  const [stage, setStage] = useState<Stage>("choose");
  const [scenario, setScenario] = useState<AttackScenario>("forged_cert");
  const [error, setError] = useState<string | null>(null);
  const [attackResult, setAttackResult] = useState<AttackerResponse | null>(null);
  const [verifyResult, setVerifyResult] = useState<VerifyResponse | null>(null);
  const [visibleStep, setVisibleStep] = useState(0);

  const run = async () => {
    setStage("attacking");
    setError(null);
    try {
      const attack = await fakeAliceAttempt(sessionId, originalDocument, scenario);
      setAttackResult(attack);
      setStage("verifying");
      setVisibleStep(0);

      const verify = await bobVerifyAttack(
        sessionId,
        attack.document,
        attack.hash_hex,
        attack.fake_signature,
        attack.fake_certificate
      );
      setVerifyResult(verify);

      for (let i = 1; i <= verify.steps.length; i++) {
        await new Promise((r) => setTimeout(r, 600));
        setVisibleStep(i);
      }
      await new Promise((r) => setTimeout(r, 300));
      setStage("done");
    } catch (e) {
      setError((e as Error).message);
      setStage("error");
    }
  };

  return (
    <main className="atk-page">
      <h1 className="atk-h1">Attacker Scenario — Fake-Alice</h1>
      <p className="atk-intro">
        Fake-Alice tries to impersonate Alice and trick Bob into accepting a
        forged document. Watch how Bob&apos;s verification catches the attack
        at the exact failure point.
      </p>

      {/* ── Scenario chooser ── */}
      <section className="atk-chooser">
        <h2 className="atk-h2">Choose Attack Type</h2>
        <div className="atk-scenarios">
          <button
            className={`atk-scenario-btn ${scenario === "forged_cert" ? "atk-active" : ""}`}
            onClick={() => setScenario("forged_cert")}
          >
            <span className="atk-scenario-icon">🎭</span>
            <div>
              <strong>Forged Certificate</strong>
              <p>Fake-Alice creates a certificate claiming to be Alice, signed with her own key instead of the CA&apos;s.</p>
              <span className="atk-fail-tag">Fails at Step 1 — Certificate Check</span>
            </div>
          </button>

          <button
            className={`atk-scenario-btn ${scenario === "tampered_doc" ? "atk-active" : ""}`}
            onClick={() => setScenario("tampered_doc")}
          >
            <span className="atk-scenario-icon">✏️</span>
            <div>
              <strong>Tampered Document</strong>
              <p>Fake-Alice modifies the document content after it was signed, then sends it with the original signature.</p>
              <span className="atk-fail-tag">Fails at Step 5 — Hash Comparison</span>
            </div>
          </button>
        </div>

        {stage === "choose" && (
          <div className="atk-gate">
            <button className="atk-btn atk-btn-danger" onClick={run}>
              Launch Attack →
            </button>
          </div>
        )}
      </section>

      {/* ── Attack packet ── */}
      {attackResult && (
        <section className="atk-packet-section">
          <h2 className="atk-h2">Fake-Alice&apos;s Forged Packet</h2>
          <div className="atk-packet">
            <div className="atk-packet-hdr">
              <span className="atk-from">Fake-Alice 🎭</span>
              <span className="atk-arrow">→</span>
              <span className="atk-to">Bob</span>
              <span className="atk-type">FORGED / {attackResult.scenario}</span>
            </div>

            <div className="atk-field">
              <span className="atk-field-label">Document Sent</span>
              <pre className="atk-field-text">{attackResult.document.slice(0, 200)}</pre>
            </div>

            <div className="atk-field">
              <span className="atk-field-label">Hash (of sent document)</span>
              <code className="atk-field-mono">{attackResult.hash_hex}</code>
            </div>

            <div className="atk-field">
              <span className="atk-field-label">Fake-Alice&apos;s Signature</span>
              <code className="atk-field-mono atk-sig">{attackResult.fake_signature.slice(0, 48)}…</code>
              <p className="atk-field-note">Signed with Fake-Alice&apos;s private key — NOT Alice&apos;s</p>
            </div>

            <div className="atk-field">
              <span className="atk-field-label">Forged Certificate</span>
              <div className="atk-cert">
                <div className="atk-cert-row"><span>Subject</span><code>{attackResult.fake_certificate.subject}</code><span className="atk-lie">← lying</span></div>
                <div className="atk-cert-row"><span>Issuer</span><code>{attackResult.fake_certificate.issuer}</code><span className="atk-lie">← forged</span></div>
                <div className="atk-cert-row"><span>Public Key (e)</span><code>{attackResult.fake_certificate.public_key.e}</code><span className="atk-attacker-key">← Fake-Alice&apos;s key</span></div>
                <div className="atk-cert-row"><span>CA Signature</span><code title={attackResult.fake_certificate.ca_signature}>{attackResult.fake_certificate.ca_signature.slice(0, 32)}…</code><span className="atk-lie">← self-signed</span></div>
              </div>
            </div>

            <div className="atk-callout">
              <span className="atk-callout-icon">⚠️</span>
              <p>{attackResult.educational_callout}</p>
            </div>
          </div>
        </section>
      )}

      {/* ── Bob's verification steps ── */}
      {(stage === "verifying" || stage === "done") && verifyResult && (
        <section className="atk-verify-section">
          <h2 className="atk-h2">Bob Runs Verification</h2>
          <div className="atk-steps">
            {verifyResult.steps.slice(0, visibleStep).map((step) => (
              <AttackStepCard key={step.index} step={step} />
            ))}
            {stage === "verifying" && visibleStep < verifyResult.steps.length && (
              <div className="atk-step-loading">
                <span className="atk-spinner">⟳</span> Step {visibleStep + 1} of {verifyResult.steps.length}…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Final result ── */}
      {stage === "done" && verifyResult && (
        <>
          <div className="atk-result-banner" role="alert">
            <span className="atk-result-icon">✗</span>
            <div>
              <strong>
                {verifyResult.failure_step === 1
                  ? "✗ Certificate Not Trusted"
                  : "✗ Signature Mismatch: Document Tampered"}
              </strong>
              <p>{verifyResult.failure_reason}</p>
              <p className="atk-security-note">
                {verifyResult.failure_step === 1
                  ? "PKI security worked: without the CA's private key, no one can forge a trusted certificate."
                  : "Integrity check worked: SHA-256 detected that the document was modified after signing."}
              </p>
            </div>
          </div>

          <div className="atk-gate">
            <button className="atk-btn" onClick={() => { setStage("choose"); setAttackResult(null); setVerifyResult(null); setVisibleStep(0); }}>
              ← Try other scenario
            </button>
            <button className="atk-btn atk-btn-primary" onClick={onContinue}>
              Continue →
            </button>
          </div>
        </>
      )}

      {stage === "error" && (
        <div className="atk-error">{error}</div>
      )}
    </main>
  );
}

function AttackStepCard({ step }: { step: VerifyStep }) {
  const isFail = !step.passed;
  return (
    <div className={`atk-step ${isFail ? "atk-step-fail" : "atk-step-ok"}`}
         aria-label={`Step ${step.index}: ${step.passed ? "passed" : "FAILED"}`}>
      <div className="atk-step-hdr">
        <span className={`atk-badge ${isFail ? "atk-badge-fail" : "atk-badge-ok"}`}>
          {isFail ? "✗" : "✓"}
        </span>
        <span className="atk-step-num">Step {step.index}</span>
        <span className="atk-step-title">{step.title}</span>
      </div>
      <p className="atk-step-desc">{step.description}</p>
      <div className="atk-formula">{step.formula}</div>
      <code className="atk-step-value">{step.value}</code>
      {isFail && (
        <div className="atk-fail-callout">
          Attack stopped here — Bob rejected the forged data.
        </div>
      )}
    </div>
  );
}
