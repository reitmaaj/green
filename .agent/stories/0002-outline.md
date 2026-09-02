# Outline stakeholder stories

## 0005 outline adopter

AS an author of strict C source
I WANT every function that reaches for control flow to be outlined so each
basic block is a file-local helper driven by an explicit program counter
SO THAT no function body hides a state machine behind unstructured
statement nesting.

## 0006 single-tool user

AS a user of the green profile
I WANT `green` itself to both outline and gate outlining
SO THAT I need exactly one tool to reach and verify the outlined shape.

## 0007 verifier of transformation output

AS a reviewer of outlined output
I WANT the emitted helpers and dispatcher to conform to every green check
(hidden control, transition boundary, braces, formatting) with no `?:`
terminators
SO THAT outlined source is green-clean by construction and re-linting never
regresses.

## 0008 idempotence seeker

AS a user who runs green twice
I WANT re-outlining an already-outlined function to be a no-op
SO THAT green accepts its own output and repeated runs do not churn source.
