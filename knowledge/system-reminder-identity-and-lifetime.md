---
{
  "schema_version": 1,
  "id": "system-reminder-identity-and-lifetime",
  "topic_key": "lifecycle.system-reminder-identity-and-lifetime",
  "scope": "common",
  "frameworks": [],
  "title": "Schedule and cancel OH reminders after the application exits",
  "area": "lifecycle",
  "summary": "Delegate due-time notifications to ReminderAgent and persist the mapping from application IDs to returned system reminder IDs; keep process-owned countdown updates separate.",
  "when_to_use": "A reminder must fire while the application runtime is frozen or absent, and the application must later replace or cancel it using its own stable identifier.",
  "keywords": [
    "reminderAgentManager",
    "ReminderAgent",
    "publishReminder",
    "cancelReminder",
    "ReminderRequestTimer",
    "PUBLISH_AGENT_REMINDER",
    "triggerTimeInSeconds",
    "系统提醒",
    "课程提醒",
    "进程退出",
    "提醒取消",
    "reminder ID"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": [
    "continuous-background-task-contract",
    "native-form-snapshot-bridge"
  ]
}
---

# Schedule and cancel OH reminders after the application exits

## When to use

A reminder must fire while the application runtime is frozen or absent, and the application must later replace or cancel it using its own stable identifier.

## Recommended approach

Delegate due-time notifications to ReminderAgent and persist the mapping from application IDs to returned system reminder IDs; keep process-owned countdown updates separate.

## Implementation steps

1. Use an SDK/backend supporting `reminderAgentManager`, declare `ohos.permission.PUBLISH_AGENT_REMINDER`, and obtain notification enablement through the appropriate native context. Report denial separately from acceptance; dismissing a permission prompt does not establish authorization.
2. Keep the desired due time in a domain record. For `ReminderRequestTimer`, convert the remaining duration to `triggerTimeInSeconds` when scheduling. Decide how overdue and sub-second deadlines behave: skip as expired, schedule a valid future request, or use an explicit immediate-notification operation. Do not label a skipped reminder as registered.
3. Await `publishReminder` and retain its returned reminder ID. An application ID or `notificationId` is not that system ID. Persist the application-to-system mapping with enough desired state to reconcile after restart; flush the store before promising durable management of the reminder.
4. Serialize replace/cancel operations for each application ID. Load the persisted mapping before cancellation and call `cancelReminder` with the recorded system ID. Define the tradeoff when replacement cancels the old reminder before registering the new one: if publication fails, return an explicit unscheduled state or restore the previous request when still valid.
5. Account for partial completion. If publication succeeds but mapping persistence fails, compensate by cancelling the returned ID or retain a recoverable pending record. After interruption or restart, compare desired records, mappings and the backend's valid-reminder query where available before rescheduling. A mapping alone does not prove a reminder still exists.
6. Keep countdown painting separate from scheduled delivery. An ArkTS `setInterval` that updates an ongoing notification runs in the application process. Persist an absolute end time for [native card refresh](native-form-snapshot-bridge.md), and use an eligible [background task](continuous-background-task-contract.md) only when the operation requires and supports one. Neither mechanism follows automatically from registering the due-time reminder.

## Conditions and alternatives

The ReminderAgent module begins at API 9; individual members and kit imports have their own SDK requirements. Check notification enablement, device capability, reminder limits and the selected request type. A duration timer and a recurring calendar/alarm request have different behavior; use the request type matching the desired clock/time-zone semantics.

If this service is unavailable, describe the implemented alternative's lifetime explicitly. A application timer is appropriate for a foreground visual update but cannot provide the same delivery contract after process exit.

## Common mistakes

Do not swallow a map-load/flush failure and claim durable cancellation support. Do not infer successful delivery from publication acceptance. Turning off a feature should await cancellation outcomes instead of clearing the local map first and losing the IDs needed for retry.

## Check it works

Publish a future reminder, exit the application, and observe delivery. Restart and cancel or replace another pending reminder using the restored mapping. Test notification denial, overdue input, repeated IDs, concurrent replacement, publication failure, map-write failure and interruption between registration and persistence. Verify that process-owned countdown updates are reported separately from system-scheduled notifications.
