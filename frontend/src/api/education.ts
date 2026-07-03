import type { RSATheoryResponse } from "../types/education";

const BASE_URL = "";

export async function fetchRSATheory(): Promise<RSATheoryResponse> {
  const res = await fetch(`${BASE_URL}/api/education/rsa-theory`);
  if (!res.ok) throw new Error(`Failed to fetch RSA theory: ${res.status}`);
  return res.json() as Promise<RSATheoryResponse>;
}
