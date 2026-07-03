export interface TLSPacket {
  index: number;
  type: string;
  from: string;
  to: string;
  description: string;
  payload: Record<string, unknown>;
  ok: boolean;
}

export interface TLSHandshakeResponse {
  session_id: string;
  mode: "normal" | "fake_alice";
  success: boolean;
  failure_reason?: string;
  packets: TLSPacket[];
}
