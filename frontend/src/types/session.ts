export interface ActorPublicKey {
  name: string;
  public_key: {
    n: string;
    e: string;
  };
  bits: number;
}

export interface SessionInitResponse {
  session_id: string;
  actors: ActorPublicKey[];
}
