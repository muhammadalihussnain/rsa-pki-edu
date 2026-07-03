import type { IssuedCertificate } from "./certificate";

export type AttackScenario = "forged_cert" | "tampered_doc";

export interface AttackerResponse {
  session_id: string;
  attacker: string;
  scenario: AttackScenario;
  description: string;
  document: string;
  hash_hex: string;
  fake_signature: string;
  educational_callout: string;
  fake_certificate: IssuedCertificate;
}
