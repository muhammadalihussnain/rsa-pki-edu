import type { TLSHandshakeResponse } from "../types/tls";

export async function runTLSHandshake(
  sessionId: string,
  injectFakeAlice: boolean = false
): Promise<TLSHandshakeResponse> {
  const res = await fetch("/api/tls/handshake", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, inject_fake_alice: injectFakeAlice }),
  });
  if (!res.ok) throw new Error(`TLS handshake failed: ${res.status}`);
  return res.json() as Promise<TLSHandshakeResponse>;
}
