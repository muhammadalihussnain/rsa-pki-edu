export interface VerifyStep {
  index: number;
  title: string;
  description: string;
  value: string;
  passed: boolean;
}

export interface VerifyResponse {
  session_id: string;
  verified: boolean;
  failure_step?: number;
  failure_reason?: string;
  steps: VerifyStep[];
}

export interface VerifyTheorySection {
  index: number;
  title: string;
  body: string;
}

export interface VerifyTheoryResponse {
  title: string;
  sections: VerifyTheorySection[];
}
