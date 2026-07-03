import type { VerifyResponse, VerifyTheoryResponse } from "../types/verification";
import type { IssuedCertificate } from "../types/certificate";

export async function fetchCertVerificationTheory(): Promise<VerifyTheoryResponse> {
  const res = await fetch("/api/education/certificate-verification");
  if (!res.ok) throw new Error(`Failed to fetch cert verification theory: ${res.status}`);
  return res.json() as Promise<VerifyTheoryResponse>;
}

export async function fetchSignatureVerificationTheory(): Promise<VerifyTheoryResponse> {
  const res = await fetch("/api/education/signature-verification");
  if (!res.ok) throw new Error(`Failed to fetch sig verification theory: ${res.status}`);
  return res.json() as Promise<VerifyTheoryResponse>;
}

export async function bobVerify(
  sessionId: string,
  document: string,
  signature: string,
  certificate: IssuedCertificate
): Promise<VerifyResponse> {
  const res = await fetch("/api/bob/verify", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, document, signature, certificate }),
  });
  if (!res.ok) throw new Error(`Verification failed: ${res.status}`);
  return res.json() as Promise<VerifyResponse>;
}
