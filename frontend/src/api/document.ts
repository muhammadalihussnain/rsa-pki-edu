import type { UploadResponse, SignResponse } from "../types/document";

export async function uploadDocument(content: string): Promise<UploadResponse> {
  const res = await fetch("/api/alice/upload-document", {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: content,
  });
  if (!res.ok) throw new Error(`Upload failed: ${res.status}`);
  return res.json() as Promise<UploadResponse>;
}

export async function signDocument(
  sessionId: string,
  hashHex: string
): Promise<SignResponse> {
  const res = await fetch("/api/alice/sign-document", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ session_id: sessionId, hash_hex: hashHex }),
  });
  if (!res.ok) throw new Error(`Sign failed: ${res.status}`);
  return res.json() as Promise<SignResponse>;
}
