import { useState } from "react";
import { initSession } from "../api/session";
import type { ActorPublicKey } from "../types/session";
import "./KeyGeneration.css";

type ActorStatus = "pending" | "generating" | "done" | "error";

interface ActorState {
  name: string;
  status: ActorStatus;
  data?: ActorPublicKey;
}

const ACTOR_NAMES = ["Alice", "Bob", "CA", "Fake-Alice"];
const ACTOR_ICONS: Record<string, string> = {
  Alice: "👩",
  Bob: "👨",
  CA: "🏛️",
  "Fake-Alice": "🎭",
};

interface Props {
  onContinue: (sessionId: string) => void;
}

export default function KeyGeneration({ onContinue }: Props) {
  const [actors, setActors] = useState<ActorState[]>(
    ACTOR_NAMES.map((name) => ({ name, status: "pending" }))
  );
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [started, setStarted] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const updateActor = (name: string, patch: Partial<ActorState>) => {
    setActors((prev) =>
      prev.map((a) => (a.name === name ? { ...a, ...patch } : a))
    );
  };

  const runGeneration = async () => {
    setStarted(true);
    setError(null);

    try {
      // Animate each actor as "generating" in sequence before the request
      for (const name of ACTOR_NAMES) {
        updateActor(name, { status: "generating" });
        await new Promise((r) => setTimeout(r, 400));
      }

      const result = await initSession();
      setSessionId(result.session_id);

      // Reveal each actor's keys one by one with a small delay
      for (const actor of result.actors) {
        updateActor(actor.name, { status: "done", data: actor });
        await new Promise((r) => setTimeout(r, 350));
      }
    } catch (e) {
      setError((e as Error).message);
      ACTOR_NAMES.forEach((n) => updateActor(n, { status: "error" }));
    }
  };

  const allDone = actors.every((a) => a.status === "done");

  return (
    <main className="keygen">
      <h1>Key Generation</h1>
      <p className="keygen-intro">
        The server will generate RSA key pairs for all four actors. Private keys
        stay on the server — only public keys are shown here.
      </p>

      {!started && (
        <button className="start-btn" onClick={runGeneration}>
          Generate Keys
        </button>
      )}

      {error && <div className="keygen-error">{error}</div>}

      <div className="actor-grid">
        {actors.map((actor) => (
          <div
            key={actor.name}
            className={`actor-card actor-${actor.status}`}
            aria-label={`${actor.name} key status: ${actor.status}`}
          >
            <div className="actor-header">
              <span className="actor-icon">{ACTOR_ICONS[actor.name]}</span>
              <span className="actor-name">{actor.name}</span>
              <span className="actor-badge">
                {actor.status === "pending" && "—"}
                {actor.status === "generating" && (
                  <span className="spinner" aria-label="generating">⟳</span>
                )}
                {actor.status === "done" && "✓"}
                {actor.status === "error" && "✗"}
              </span>
            </div>

            {actor.status === "generating" && (
              <p className="actor-progress">
                Generating {actor.name}&apos;s keys…
              </p>
            )}

            {actor.status === "done" && actor.data && (
              <div className="actor-keys">
                <div className="key-row">
                  <span className="key-label">Bits</span>
                  <code className="key-value">{actor.data.bits}</code>
                </div>
                <div className="key-row">
                  <span className="key-label">e</span>
                  <code className="key-value">{actor.data.public_key.e}</code>
                </div>
                <div className="key-row">
                  <span className="key-label">n</span>
                  <code className="key-value key-n" title={actor.data.public_key.n}>
                    {actor.data.public_key.n.slice(0, 24)}…
                  </code>
                </div>
              </div>
            )}
          </div>
        ))}
      </div>

      {allDone && sessionId && (
        <div className="keygen-done">
          <p className="session-id">
            Session: <code>{sessionId}</code>
          </p>
          <button
            className="continue-btn"
            onClick={() => onContinue(sessionId)}
          >
            Continue →
          </button>
        </div>
      )}
    </main>
  );
}
