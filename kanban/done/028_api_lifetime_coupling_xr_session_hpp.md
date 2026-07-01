# Express XrInstance lifetime dependency of XrSessionObj in the API

**File:** `components/xr_session/xr_session.hpp`
**Principle:** Handle/pointer lifetime dependencies must be expressed in the type system or API contract, not left implicit; callers must not be able to create lifetime inversions without the API objecting.

`XrSessionObj::create()` stores the `XrInstance` parameter as the public member `instance`, which must outlive the session. This storage is implicit — the caller passes a plain `XrInstance` value and nothing in the type system prevents the instance from being destroyed before the session. The public `instance` member is also writable by anyone. At minimum: (1) rename the stored member to `instance_` and make it private, (2) document the lifetime requirement in the header, and (3) consider storing a `const XrInstance` (already non-owning) so the read-only intent is clear.

## Acceptance
- The `instance` member of `XrSessionObj` is private (renamed `instance_`).
- A `create()` doc comment states that the provided `XrInstance` must outlive this object.
- No external code accesses the stored instance directly (internal use in `poll_events`/`create` via `instance_`).
- Build succeeds with no new warnings.
