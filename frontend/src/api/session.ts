import type { SessionInitResponse } from "../types/session";

export async function initSession(): Promise<SessionInitResponse> {
  const res = await fetch("/api/session/init", { method: "POST" });
  if (!res.ok) throw new Error(`Session init failed: ${res.status}`);
  return res.json() as Promise<SessionInitResponse>;
}
