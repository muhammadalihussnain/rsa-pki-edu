import { useState } from "react";
import RSATheory from "./pages/RSATheory";
import KeyGeneration from "./pages/KeyGeneration";
import CACertificate from "./pages/CACertificate";
import SignDocument from "./pages/SignDocument";
import BobVerify from "./pages/BobVerify";
import AttackerScenario from "./pages/AttackerScenario";
import TLSHandshake from "./pages/TLSHandshake";
import type { IssuedCertificate } from "./types/certificate";
import type { SignResponse } from "./types/document";

type Screen =
  | "theory"
  | "keygen"
  | "ca-cert"
  | "sign"
  | "bob-verify"
  | "attacker"
  | "tls"
  | "done";

export default function App() {
  const [screen, setScreen] = useState<Screen>("theory");
  const [sessionId, setSessionId] = useState<string>("");
  const [certificate, setCertificate] = useState<IssuedCertificate | null>(null);
  const [signResult, setSignResult] = useState<SignResponse | null>(null);
  const [documentContent, setDocumentContent] = useState<string>("");

  if (screen === "theory") {
    return <RSATheory onContinue={() => setScreen("keygen")} />;
  }

  if (screen === "keygen") {
    return (
      <KeyGeneration
        onContinue={(sid) => { setSessionId(sid); setScreen("ca-cert"); }}
      />
    );
  }

  if (screen === "ca-cert") {
    return (
      <CACertificate
        sessionId={sessionId}
        onContinue={(cert) => { setCertificate(cert); setScreen("sign"); }}
      />
    );
  }

  if (screen === "sign" && certificate) {
    return (
      <SignDocument
        sessionId={sessionId}
        certificate={certificate}
        onContinue={(result, content) => {
          setSignResult(result);
          setDocumentContent(content);
          setScreen("bob-verify");
        }}
      />
    );
  }

  if (screen === "bob-verify" && certificate && signResult) {
    return (
      <BobVerify
        sessionId={sessionId}
        document={documentContent}
        signResult={signResult}
        certificate={certificate}
        onContinue={() => setScreen("attacker")}
      />
    );
  }

  if (screen === "attacker") {
    return (
      <AttackerScenario
        sessionId={sessionId}
        originalDocument={documentContent}
        onContinue={() => setScreen("tls")}
      />
    );
  }

  if (screen === "tls") {
    return (
      <TLSHandshake
        sessionId={sessionId}
        onContinue={() => setScreen("done")}
      />
    );
  }

  return (
    <main className="done-page">
      <div className="done-card">
        <span className="done-icon">🎓</span>
        <h2>Walkthrough Complete</h2>
        <p>
          You have completed all modules: RSA theory, key generation,
          certificate authority, document signing, verification, attacker
          scenarios, and TLS simulation.
        </p>
        <div className="done-summary">
          <div className="done-item">✓ RSA key generation understood</div>
          <div className="done-item">✓ Certificate chain verified</div>
          <div className="done-item">✓ Document signed with SHA-256 + RSA</div>
          <div className="done-item">✓ Bob's 5-step verification passed</div>
          <div className="done-item">✓ Forged certificate attack detected</div>
          <div className="done-item">✓ TLS handshake with HMAC completed</div>
        </div>
        <button
          className="done-restart"
          onClick={() => setScreen("theory")}
        >
          ← Start over
        </button>
      </div>
    </main>
  );
}
