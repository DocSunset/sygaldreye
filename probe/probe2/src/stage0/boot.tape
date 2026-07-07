# The boot tape (ADR-028): what a hosted peer instates, by choice, before
# anything else exists. Replayed by the crown; no parser, codec, or
# resolver is needed to read this.
NODE parser0 parser
NODE resolver0 naive_resolver
NODE registry0 registry-face
NODE engine0 slot
NODE super0 supervisor
SET super0/restart-limit 100
