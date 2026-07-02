export interface RSAFlavour {
  name: string;
  full_name: string;
  use: string;
  security: string;
  note: string;
}

export interface WhatIsRSA {
  title: string;
  body: string;
  uses: string[];
}

export interface TheoryStep {
  index: number;
  title: string;
  explanation: string;
  condition: string;
}

export interface ExampleStep {
  label: string;
  condition: string;
  note: string;
}

export interface KeyPair {
  e?: number;
  d?: number;
  n: number;
}

export interface RSAExample {
  p: number;
  q: number;
  n: number;
  phi: number;
  e: number;
  d: number;
  steps: ExampleStep[];
  public_key: KeyPair;
  private_key: KeyPair;
  verify: string;
}

export interface RSATheoryResponse {
  what_is_rsa: WhatIsRSA;
  flavours: RSAFlavour[];
  steps: TheoryStep[];
  example: RSAExample;
}
