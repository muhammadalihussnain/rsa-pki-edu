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

type Stage =
  | "loading"
  | "cert-theory"
  | "sig-theory"
  | "verifying"
  | "done"
  | "error";

interface Props {
  sessionId: string;
  document: string;
  signResult: SignResponse;
  certificate: IssuedCertificate;
  onContinue: () => void;
}

export default function BobVerify({
  sessionId,
  document,
  signResult,
  certificate,
  onContinue,
}: Props) {
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

  useEffect(() => {
    Promise.all([
      fetchCertVerificationTheory(),
      fetchSignatureVerificationTheory(),
    ])
      .then(([cert, sig]) => {
        setCertTheory(cert);
        setSigTheory(sig);
        setStage("cert-theory");
      })
      .catch((e: Error) => {
        setError(e.message);
        setStage("error");
      });
  }, []);

  const runVerification = async () => {
    setStage("verifying");
    setVisibleStep(0);
    try {
      const result = await bobVerify(
        sessionId,
        document,
        signResult.original_hash,
        signResult.signature,
        certificate
      );
      setVerifySteps(result.steps);
      setVerified(result.verified);
      setFailureReason(result.failure_reason ?? "");

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
        Bob received Alice&apos;s document and signature. Before running the
        verification, read through how each check works.
      </p>

      {/* ── What Bob received ── */}
      <div className="bv-received">
        <div className="bv-recv-item">
          <span className="bv-recv-label">Document</span>
          <span className="bv-recv-val">{document.slice(0, 60)}{document.length > 60 ? "…" : ""}</span>
        </div>
        <div className="bv-recv-item">
          <span className="bv-recv-label">Certificate</span>
          <span className="bv-recv-val">Subject: {certificate.subject} · Issuer: {certificate.issuer} · Serial: {certificate.serial}</span>
        </div>
        <div className="bv-recv-item">
          <span className="bv-recv-label">Signature</span>
          <code className="bv-recv-val bv-mono">{signResult.signature.slice(0, 32)}…</code>
        </div>
      </div>

      {/* ── Certificate Verification Theory ── */}
      {certTheory && (
        <section className="bv-theory-section">
          <div className="bv-theory-header">
            <span className="bv-theory-num">T1</span>
            <span className="bv-theory-title">{certTheory.title}</span>
          </div>
          <ol className="bv-theory-list">
            {certTheory.sections.map((s) => (
              <li
                key={s.index}
                className={`bv-theory-item ${activeCertSection === s.index ? "bv-theory-open" : ""}`}
                onClick={() => setActiveCertSection(activeCertSection === s.index ? null : s.index)}
              >
                <div className="bv-theory-item-header">
                  <span className="bv-theory-item-num">{s.index}</span>
                  <span className="bv-theory-item-title">{s.title}</span>
                  <span className="bv-chevron">{activeCertSection === s.index ? "▲" : "▼"}</span>
                </div>
                {activeCertSection === s.index && (
                  <p className="bv-theory-body">{s.body}</p>
                )}
              </li>
            ))}
          </ol>
        </section>
      )}

      {/* ── Gate 1 → show sig theory ── */}
      {stage === "cert-theory" && (
        <div className="bv-gate">
          <button className="bv-btn" onClick={() => setStage("sig-theory")}>
            I understand — continue to signature theory →
          </button>
        </div>
      )}

      {/* ── Signature Verification Theory ── */}
      {(stage === "sig-theory" || stage === "verifying" || stage === "done") && sigTheory && (
        <section className="bv-theory-section">
          <div className="bv-theory-header">
            <span className="bv-theory-num">T2</span>
            <span className="bv-theory-title">{sigTheory.title}</span>
          </div>
          <ol className="bv-theory-list">
            {sigTheory.sections.map((s) => (
              <li
                key={s.index}
                className={`bv-theory-item ${activeSigSection === s.index ? "bv-theory-open" : ""}`}
                onClick={() => setActiveSigSection(activeSigSection === s.index ? null : s.index)}
              >
                <div className="bv-theory-item-header">
                  <span className="bv-theory-item-num">{s.index}</span>
                  <span className="bv-theory-item-title">{s.title}</span>
                  <span className="bv-chevron">{activeSigSection === s.index ? "▲" : "▼"}</span>
                </div>
                {activeSigSection === s.index && (
                  <p className="bv-theory-body">{s.body}</p>
                )}
              </li>
            ))}
          </ol>
        </section>
      )}

      {/* ── Gate 2 → run verification ── */}
      {stage === "sig-theory" && (
        <div className="bv-gate">
          <button className="bv-btn" onClick={runVerification}>
            Run Verification →
          </button>
        </div>
      )}

      {/* ── Animated verification steps ── */}
      {(stage === "verifying" || stage === "done") && (
        <section className="bv-verify-section">
          <h2 className="bv-h2">Verification Steps</h2>
          <div className="bv-steps">
            {verifySteps.slice(0, visibleStep).map((step) => (
              <VerifyStepCard key={step.index} step={step} />
            ))}
            {stage === "verifying" && visibleStep < 5 && (
              <div className="bv-step-loading">
                <span className="bv-spinner">⟳</span> Running step {visibleStep + 1}…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Final result banner ── */}
      {stage === "done" && verified !== null && (
        <div className={`bv-result-banner ${verified ? "bv-success" : "bv-fail"}`}
             role="alert"
             aria-live="polite">
          {verified ? (
            <>
              <span className="bv-result-icon">✓</span>
              <div>
                <strong>Verified: This document is from Alice</strong>
                <p>The certificate is trusted, the signature is valid, and the document is unmodified.</p>
              </div>
            </>
          ) : (
            <>
              <span className="bv-result-icon">✗</span>
              <div>
                <strong>Verification Failed</strong>
                <p>{failureReason || "One or more verification checks failed."}</p>
              </div>
            </>
          )}
        </div>
      )}

      {stage === "done" && (
        <div className="bv-gate">
          <button className="bv-btn" onClick={onContinue}>
            Continue →
          </button>
        </div>
      )}
    </main>
  );
}

function VerifyStepCard({ step }: { step: VerifyStep }) {
  return (
    <div className={`bv-step-card ${step.passed ? "bv-step-pass" : "bv-step-fail"}`}
         aria-label={`Step ${step.index}: ${step.passed ? "passed" : "failed"}`}>
      <div className="bv-step-header">
        <span className={`bv-step-badge ${step.passed ? "bv-badge-pass" : "bv-badge-fail"}`}>
          {step.passed ? "✓" : "✗"}
        </span>
        <span className="bv-step-index">Step {step.index}</span>
        <span className="bv-step-title">{step.title}</span>
      </div>
      <p className="bv-step-desc">{step.description}</p>
      <code className="bv-step-value">{step.value}</code>
    </div>
  );
}
