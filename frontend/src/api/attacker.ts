import type { AttackerResponse, AttackScenario } from "../types/attacker";
import type { VerifyResponse } from "../types/verification";
import type { IssuedCertificate } from "../types/certificate";

export async function fakeAliceAttempt(
  sessionId: string,
  document: string,
  scenario: AttackScenario
): Promise<AttackerResponse> {
  const res = await fetch("/api/fake-alice/attempt", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, document, scenario }),
  });
  if (!res.ok) throw new Error(`Attacker request failed: ${res.status}`);
  return res.json() as Promise<AttackerResponse>;
}

export async function bobVerifyAttack(
  sessionId: string,
  document: string,
  hashHex: string,
  signature: string,
  certificate: IssuedCertificate
): Promise<VerifyResponse> {
  const res = await fetch("/api/bob/verify", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, document, hash_hex: hashHex, signature, certificate }),
  });
  if (!res.ok) throw new Error(`Verification failed: ${res.status}`);
  return res.json() as Promise<VerifyResponse>;
}
