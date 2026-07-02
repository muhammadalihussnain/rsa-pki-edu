import type { CertTheoryResponse, IssueCertResponse } from "../types/certificate";

export async function fetchCertTheory(): Promise<CertTheoryResponse> {
  const res = await fetch("/api/education/certificate-theory");
  if (!res.ok) throw new Error(`Failed to fetch certificate theory: ${res.status}`);
  return res.json() as Promise<CertTheoryResponse>;
}

export async function issueCertificate(
  sessionId: string,
  subject: string
): Promise<IssueCertResponse> {
  const res = await fetch("/api/ca/issue-certificate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, subject }),
  });
  if (!res.ok) throw new Error(`Certificate issuance failed: ${res.status}`);
  return res.json() as Promise<IssueCertResponse>;
}
