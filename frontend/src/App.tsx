import { useState } from "react";
import RSATheory from "./pages/RSATheory";
import KeyGeneration from "./pages/KeyGeneration";
import CACertificate from "./pages/CACertificate";
import SignDocument from "./pages/SignDocument";
import BobVerify from "./pages/BobVerify";
import type { IssuedCertificate } from "./types/certificate";
import type { SignResponse } from "./types/document";

type Screen = "theory" | "keygen" | "ca-cert" | "sign" | "bob-verify" | "done";

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
        onContinue={(sid) => {
          setSessionId(sid);
          setScreen("ca-cert");
        }}
      />
    );
  }

  if (screen === "ca-cert") {
    return (
      <CACertificate
        sessionId={sessionId}
        onContinue={(cert) => {
          setCertificate(cert);
          setScreen("sign");
        }}
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
        onContinue={() => setScreen("done")}
      />
    );
  }

  return (
    <main style={{ padding: "3rem", textAlign: "center", fontFamily: "system-ui" }}>
      <h2>Phase 6 complete — Phase 7 coming next</h2>
      <p style={{ color: "var(--text)", marginTop: "0.5rem" }}>
        Session: <code>{sessionId}</code>
      </p>
      <button
        onClick={() => setScreen("theory")}
        style={{
          marginTop: "1.5rem",
          background: "var(--accent)",
          color: "#fff",
          border: "none",
          padding: "0.65rem 1.4rem",
          borderRadius: "8px",
          cursor: "pointer",
          fontSize: "0.95rem",
        }}
      >
        ← Start over
      </button>
    </main>
  );
}
