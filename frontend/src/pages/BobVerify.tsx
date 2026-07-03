import { useEffect, useState } from "react";
import {
  fetchCertVerificationTheory,
  fetchSignatureVerificationTheory,
  bobVerify,
} from "../api/verification";
import type { VerifyTheoryResponse, VerifyStep } from "../types/verification";
import type { IssuedCertificate } from "../types/certificate";
import type { SignResponse } from "../types/document";
import "./BobVerify.css";

type Stage = "loading" | "cert-theory" | "sig-theory" | "verifying" | "done" | "error";

interface Props {
  sessionId: string;
  document: string;
  signResult: SignResponse;
  certificate: IssuedCertificate;
  onContinue: () => void;
}

export default function BobVerify({ sessionId, document, signResult, certificate, onContinue }: Props) {
  const [stage, setStage] = useState<Stage>("loading");
  const [error, setError] = useState<string | null>(null);
  const [certTheory, setCertTheory] = useState<VerifyTheoryResponse | null>(null);
  const [sigTheory, setSigTheory] = useState<VerifyTheoryResponse | null>(null);
  const [activeCertSection, setActiveCertSection] = useState<number | null>(null);
  const [activeSigSection, setActiveSigSection] = useState<number | null>(null);
  const [verifySteps, setVerifySteps] = useState<VerifyStep[]>([]);
  const [visibleStep, setVisibleStep] = useState(0);
  const [verified, setVerified] = useState<boolean | null>(null);
  const [failureReason, setFailureReason] = useState<string>("");
  const [aliceSignature, setAliceSignature] = useState<string>("");
  const [sigExpanded, setSigExpanded] = useState(false);

  useEffect(() => {
    Promise.all([fetchCertVerificationTheory(), fetchSignatureVerificationTheory()])
      .then(([cert, sig]) => { setCertTheory(cert); setSigTheory(sig); setStage("cert-theory"); })
      .catch((e: Error) => { setError(e.message); setStage("error"); });
  }, []);

  const runVerification = async () => {
    setStage("verifying");
    setVisibleStep(0);
    try {
      const result = await bobVerify(sessionId, document, signResult.original_hash, signResult.signature, certificate);
      setVerifySteps(result.steps);
      setVerified(result.verified);
      setFailureReason(result.failure_reason ?? "");
      setAliceSignature(result.alice_signature ?? signResult.signature);
      for (let i = 1; i <= result.steps.length; i++) {
        await new Promise((r) => setTimeout(r, 700));
        setVisibleStep(i);
      }
      await new Promise((r) => setTimeout(r, 300));
      setStage("done");
    } catch (e) {
      setError((e as Error).message);
      setStage("error");
    }
  };

  if (stage === "loading") return <div className="bv-state">Loading theory…</div>;
  if (stage === "error")   return <div className="bv-state bv-error">Error: {error}</div>;

  return (
    <main className="bv-page">
      <h1 className="bv-h1">Bob&apos;s Verification</h1>
      <p className="bv-intro">
        Bob receives a packet from Alice containing her document, certificate,
        and digital signature. He uses only <strong>public keys</strong> to
        verify everything — no private key is needed on his side.
      </p>

      {/* ── Packet Alice sent ── */}
      <section className="bv-packet" aria-label="Packet received from Alice">
        <div className="bv-packet-header">
          <span className="bv-packet-from">Alice</span>
          <span className="bv-packet-arrow">→</span>
          <span className="bv-packet-to">Bob</span>
          <span className="bv-packet-type">SignedDocument</span>
        </div>

        <div className="bv-packet-body">
          {/* Document */}
          <div className="bv-field">
            <span className="bv-field-label">Document</span>
            <pre className="bv-field-text">
              {document.slice(0, 200)}{document.length > 200 ? "\n…" : ""}
            </pre>
          </div>

          {/* Hash */}
          <div className="bv-field">
            <span className="bv-field-label">SHA-256 Hash</span>
            <code className="bv-field-mono">{signResult.original_hash}</code>
          </div>

          {/* Signature — expandable */}
          <div className="bv-field">
            <div className="bv-field-sig-header">
              <span className="bv-field-label">Alice&apos;s Signature</span>
              <button
                className="bv-sig-toggle"
                onClick={() => setSigExpanded((v) => !v)}
                aria-expanded={sigExpanded}
              >
                {sigExpanded ? "Hide" : "Show full"}
              </button>
            </div>
            <code className={`bv-field-mono bv-sig-val ${sigExpanded ? "bv-sig-expanded" : ""}`}>
              {sigExpanded
                ? signResult.signature
                : signResult.signature.slice(0, 48) + "…"}
            </code>
            <p className="bv-sig-note">
              sig = hash_int ^ d_Alice mod n_Alice &nbsp;·&nbsp; Only Alice could produce this
            </p>
          </div>

          {/* Certificate summary */}
          <div className="bv-field">
            <span className="bv-field-label">Certificate</span>
            <div className="bv-cert-summary">
              <div className="bv-cert-row"><span>Subject</span><code>{certificate.subject}</code></div>
              <div className="bv-cert-row"><span>Issuer</span><code>{certificate.issuer}</code></div>
              <div className="bv-cert-row"><span>Serial</span><code>{certificate.serial}</code></div>
              <div className="bv-cert-row"><span>Valid to</span><code>{certificate.valid_to}</code></div>
              <div className="bv-cert-row">
                <span>Public Key (e)</span>
                <code>{certificate.public_key.e}</code>
              </div>
              <div className="bv-cert-row">
                <span>Public Key (n)</span>
                <code title={certificate.public_key.n}>
                  {certificate.public_key.n.slice(0, 32)}…
                </code>
              </div>
              <div className="bv-cert-row">
                <span>CA Signature</span>
                <code title={certificate.ca_signature}>
                  {certificate.ca_signature.slice(0, 32)}…
                </code>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* ── Theory T1: Certificate verification ── */}
      {certTheory && (
        <TheoryBlock
          tag="T1"
          title={certTheory.title}
          sections={certTheory.sections}
          active={activeCertSection}
          onToggle={setActiveCertSection}
        />
      )}

      {stage === "cert-theory" && (
        <div className="bv-gate">
          <button className="bv-btn" onClick={() => setStage("sig-theory")}>
            I understand certificate verification →
          </button>
        </div>
      )}

      {/* ── Theory T2: Signature verification ── */}
      {(stage === "sig-theory" || stage === "verifying" || stage === "done") && sigTheory && (
        <TheoryBlock
          tag="T2"
          title={sigTheory.title}
          sections={sigTheory.sections}
          active={activeSigSection}
          onToggle={setActiveSigSection}
        />
      )}

      {stage === "sig-theory" && (
        <div className="bv-gate">
          <button className="bv-btn" onClick={runVerification}>
            Run Verification →
          </button>
        </div>
      )}

      {/* ── Animated verification steps ── */}
      {(stage === "verifying" || stage === "done") && (
        <section className="bv-steps-section">
          <h2 className="bv-h2">Verification — Step by Step</h2>
          <div className="bv-steps">
            {verifySteps.slice(0, visibleStep).map((step) => (
              <StepCard key={step.index} step={step} />
            ))}
            {stage === "verifying" && visibleStep < 5 && (
              <div className="bv-step-loading">
                <span className="bv-spinner">⟳</span>
                Running step {visibleStep + 1} of 5…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Final result ── */}
      {stage === "done" && verified !== null && (
        <>
          {aliceSignature && (
            <div className="bv-sig-recap">
              <span className="bv-sig-recap-label">Alice&apos;s Signature (full)</span>
              <code className="bv-sig-recap-val">{aliceSignature}</code>
            </div>
          )}

          <div className={`bv-result ${verified ? "bv-result-ok" : "bv-result-fail"}`} role="alert">
            <span className="bv-result-icon">{verified ? "✓" : "✗"}</span>
            <div>
              <strong>
                {verified
                  ? "Verified: This document is from Alice"
                  : "Verification Failed"}
              </strong>
              <p>
                {verified
                  ? "Certificate trusted · Signature valid · Document unmodified"
                  : (failureReason || "One or more checks failed")}
              </p>
            </div>
          </div>

          <div className="bv-gate">
            <button className="bv-btn" onClick={onContinue}>Continue →</button>
          </div>
        </>
      )}
    </main>
  );
}

/* ── Sub-components ───────────────────────────────────────────────────────── */

function TheoryBlock({
  tag, title, sections, active, onToggle
}: {
  tag: string;
  title: string;
  sections: VerifyTheoryResponse["sections"];
  active: number | null;
  onToggle: (i: number | null) => void;
}) {
  return (
    <section className="bv-theory">
      <div className="bv-theory-hdr">
        <span className="bv-theory-tag">{tag}</span>
        <span className="bv-theory-title">{title}</span>
      </div>
      <ol className="bv-theory-list">
        {sections.map((s) => (
          <li
            key={s.index}
            className={`bv-titem ${active === s.index ? "bv-titem-open" : ""}`}
            onClick={() => onToggle(active === s.index ? null : s.index)}
          >
            <div className="bv-titem-hdr">
              <span className="bv-titem-num">{s.index}</span>
              <span className="bv-titem-title">{s.title}</span>
              <span className="bv-chevron">{active === s.index ? "▲" : "▼"}</span>
            </div>
            {active === s.index && <p className="bv-titem-body">{s.body}</p>}
          </li>
        ))}
      </ol>
    </section>
  );
}

function StepCard({ step }: { step: VerifyStep }) {
  return (
    <div className={`bv-step ${step.passed ? "bv-step-ok" : "bv-step-fail"}`}>
      <div className="bv-step-hdr">
        <span className={`bv-badge ${step.passed ? "bv-badge-ok" : "bv-badge-fail"}`}>
          {step.passed ? "✓" : "✗"}
        </span>
        <span className="bv-step-num">Step {step.index}</span>
        <span className="bv-step-title">{step.title}</span>
      </div>
      <p className="bv-step-desc">{step.description}</p>
      <div className="bv-formula">{step.formula}</div>
      <code className="bv-step-value">{step.value}</code>
    </div>
  );
}
