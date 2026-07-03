import { useRef, useState } from "react";
import { uploadDocument, signDocument } from "../api/document";
import type { UploadResponse, SignResponse } from "../types/document";
import type { IssuedCertificate } from "../types/certificate";
import "./SignDocument.css";

type Stage = "upload" | "uploaded" | "signing" | "signed" | "error";

interface Props {
  sessionId: string;
  certificate: IssuedCertificate;
  onContinue: (signResult: SignResponse, documentContent: string) => void;
}

export default function SignDocument({ sessionId, certificate, onContinue }: Props) {
  const fileInputRef = useRef<HTMLInputElement>(null);
  const [stage, setStage] = useState<Stage>("upload");
  const [error, setError] = useState<string | null>(null);
  const [uploadResult, setUploadResult] = useState<UploadResponse | null>(null);
  const [signResult, setSignResult] = useState<SignResponse | null>(null);
  const [visibleStep, setVisibleStep] = useState(0);

  const handleFile = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setError(null);
    try {
      const text = await file.text();
      const result = await uploadDocument(text);
      setUploadResult(result);
      setStage("uploaded");
    } catch (err) {
      setError((err as Error).message);
      setStage("error");
    }
  };

  const handleSign = async () => {
    if (!uploadResult) return;
    setStage("signing");
    setVisibleStep(0);
    try {
      const result = await signDocument(sessionId, uploadResult.sha256_hash);
      setSignResult(result);
      // Reveal steps one by one
      for (let i = 1; i <= result.steps.length; i++) {
        await new Promise((r) => setTimeout(r, 600));
        setVisibleStep(i);
      }
      setStage("signed");
    } catch (err) {
      setError((err as Error).message);
      setStage("error");
    }
  };

  return (
    <main className="sd-page">
      <h1 className="sd-h1">Document Upload &amp; Signing</h1>
      <p className="sd-intro">
        Alice uploads a document, computes its SHA-256 hash, then signs the
        hash with her RSA private key. The signature proves the document came
        from Alice and has not been tampered with.
      </p>

      {/* ── Certificate reference ── */}
      <div className="sd-cert-ref">
        <span className="sd-cert-label">Alice&apos;s Certificate</span>
        <span className="sd-cert-serial">Serial: <code>{certificate.serial}</code></span>
        <span className="sd-cert-issuer">Issuer: {certificate.issuer}</span>
        <span className="sd-cert-valid">Valid to: {certificate.valid_to}</span>
      </div>

      {/* ── Step 1: Upload ── */}
      <section className="sd-section">
        <div className="sd-step-header">
          <span className="sd-step-num">1</span>
          <span className="sd-step-title">Upload a text document</span>
        </div>

        {stage === "upload" && (
          <div className="sd-upload-zone" onClick={() => fileInputRef.current?.click()}
               role="button" tabIndex={0}
               onKeyDown={(e) => e.key === "Enter" && fileInputRef.current?.click()}
               aria-label="Click to select a text file">
            <div className="sd-upload-icon">📄</div>
            <p className="sd-upload-hint">Click to select a .txt file</p>
            <input
              ref={fileInputRef}
              type="file"
              accept=".txt,text/plain"
              onChange={handleFile}
              className="sd-file-input"
              aria-hidden="true"
            />
          </div>
        )}

        {uploadResult && (
          <div className="sd-doc-preview">
            <div className="sd-doc-meta">
              <span className="sd-doc-length">{uploadResult.content_length} bytes</span>
            </div>
            <pre className="sd-doc-content" aria-label="Document content">
              {uploadResult.content.slice(0, 500)}
              {uploadResult.content.length > 500 && "\n…(truncated)"}
            </pre>
          </div>
        )}
      </section>

      {/* ── Step 2: Hash ── */}
      {uploadResult && (
        <section className="sd-section">
          <div className="sd-step-header">
            <span className="sd-step-num">2</span>
            <span className="sd-step-title">SHA-256 Hash</span>
          </div>
          <p className="sd-explain">
            A SHA-256 digest uniquely fingerprints the document. Even a single
            character change produces a completely different hash.
          </p>
          <div className="sd-hash-box" aria-label="SHA-256 hash">
            <span className="sd-hash-label">SHA-256</span>
            <code className="sd-hash-value">{uploadResult.sha256_hash}</code>
          </div>

          {stage === "uploaded" && (
            <button className="sd-btn" onClick={handleSign}>
              Sign with Alice&apos;s private key →
            </button>
          )}
        </section>
      )}

      {/* ── Step 3: Signing process ── */}
      {(stage === "signing" || stage === "signed") && signResult && (
        <section className="sd-section">
          <div className="sd-step-header">
            <span className="sd-step-num">3</span>
            <span className="sd-step-title">Signing Process</span>
          </div>

          <div className="sd-sign-steps">
            {signResult.steps.slice(0, visibleStep).map((s) => (
              <div key={s.index} className="sd-sign-step" aria-label={s.title}>
                <div className="sd-sign-step-header">
                  <span className="sd-sign-num">{s.index}</span>
                  <span className="sd-sign-title">{s.title}</span>
                </div>
                <p className="sd-sign-desc">{s.description}</p>
                <code className="sd-sign-value">{s.value}</code>
              </div>
            ))}
            {stage === "signing" && visibleStep < 3 && (
              <div className="sd-signing-loader">
                <span className="sd-spinner">⟳</span> Signing…
              </div>
            )}
          </div>
        </section>
      )}

      {/* ── Step 4: Result ── */}
      {stage === "signed" && signResult && (
        <section className="sd-section">
          <div className="sd-step-header">
            <span className="sd-step-num">4</span>
            <span className="sd-step-title">Signature Ready</span>
          </div>

          <div className="sd-result-card">
            <div className="sd-result-row">
              <span className="sd-result-label">Signer</span>
              <span className="sd-result-value">{signResult.signer}</span>
            </div>
            <div className="sd-result-row">
              <span className="sd-result-label">Document Hash</span>
              <code className="sd-result-value sd-truncate" title={signResult.original_hash}>
                {signResult.original_hash}
              </code>
            </div>
            <div className="sd-result-row">
              <span className="sd-result-label">Signature</span>
              <code className="sd-result-value sd-truncate" title={signResult.signature}>
                {signResult.signature.slice(0, 40)}…
              </code>
            </div>
            <div className="sd-result-banner">
              ✓ Document signed — ready to send to Bob
            </div>
          </div>

          <div className="sd-gate">
            <button className="sd-btn" onClick={() => onContinue(signResult, uploadResult?.content ?? "")}>
              Send to Bob →
            </button>
          </div>
        </section>
      )}

      {stage === "error" && (
        <div className="sd-error">{error}</div>
      )}
    </main>
  );
}
