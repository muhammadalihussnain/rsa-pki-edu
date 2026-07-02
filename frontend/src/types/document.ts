export interface UploadResponse {
  content: string;
  content_length: number;
  sha256_hash: string;
}

export interface SignStep {
  index: number;
  title: string;
  description: string;
  value: string;
}

export interface SignResponse {
  session_id: string;
  original_hash: string;
  hash_as_integer: string;
  signature: string;
  signer: string;
  steps: SignStep[];
}
