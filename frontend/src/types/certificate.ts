export interface CertTheorySection {
  index: number;
  title: string;
  body: string;
}

export interface MockCertificate {
  subject: string;
  issuer: string;
  public_key: { e: string; n: string };
  serial: string;
  valid_from: string;
  valid_to: string;
  ca_signature: string;
  field_explanations: Record<string, string>;
}

export interface CertTheoryResponse {
  sections: CertTheorySection[];
  mock_certificate: MockCertificate;
}

export interface IssuedCertificate {
  subject: string;
  issuer: string;
  public_key: { n: string; e: string };
  serial: string;
  valid_from: string;
  valid_to: string;
  cert_hash: string;
  ca_signature: string;
}

export interface Packet {
  index: number;
  from: string;
  to: string;
  type: string;
  description: string;
  payload: Record<string, unknown>;
}

export interface IssueCertResponse {
  session_id: string;
  packets: Packet[];
  certificate: IssuedCertificate;
}
