import { useState } from "react";
import RSATheory from "./pages/RSATheory";
import KeyGeneration from "./pages/KeyGeneration";
import CACertificate from "./pages/CACertificate";

type Screen = "theory" | "keygen" | "ca-cert" | "done";

export default function App() {
  const [screen, setScreen] = useState<Screen>("theory");
  const [sessionId, setSessionId] = useState<string>("");

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
        onContinue={() => setScreen("done")}
      />
    );
  }

  return (
    <main style={{ padding: "3rem", textAlign: "center", fontFamily: "system-ui" }}>
      <h2>Phase 4 complete — Phase 5 coming next</h2>
      <p style={{ color: "#555" }}>Session: <code>{sessionId}</code></p>
      <button onClick={() => setScreen("theory")} style={{ marginTop: "1rem" }}>
        ← Start over
      </button>
    </main>
  );
}
