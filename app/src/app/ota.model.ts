export enum OtaStatus {
  Idle = 0,
  Started = 1,
  Completed = 2,
  Failed = 3
}

export interface OtaState {
  status: OtaStatus;
  totalBytesExpected: number;
  totalBytesReceived: number;
}

export type OtaStatusString =
  | "Idle"
  | "Update in progress"
  | "Update completed successfully"
  | "Update failed";

export function otaStatusToString(status: OtaStatus): OtaStatusString {
  switch (status) {
    case OtaStatus.Idle:
      return "Idle";
    case OtaStatus.Started:
      return "Update in progress";
    case OtaStatus.Completed:
      return "Update completed successfully";
    case OtaStatus.Failed:
      return "Update failed";
  }
}
