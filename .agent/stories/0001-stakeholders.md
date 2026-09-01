# Stakeholders

## 0000 profile author

AS the author of a reusable C source profile
I WANT a single tool that proves a source tree inhabits the strict C89 ∩ C23
intersection under both GCC and Clang with explicit semantic structure
SO THAT I can maintain one dialect intersection instead of several dialects.

## 0001 project maintainer

AS a project maintainer targeting long-lived C source
I WANT the toolchain to run seven independent checks per translation unit
and report a final matrix
SO THAT I know precisely which compiler/standard combination fails before release.

## 0002 toolchain integrator

AS an integrator of a prebuilt binary toolchain
I WANT `green doctor` to validate compiler paths, versions, C89/C23
capability, warning-option capability, compilation-database status, and
plugin ABI match
SO THAT an ABI-mismatched plugin is detected as fatal rather than silently wrong.

## 0003 platform developer

AS a systems programmer needing representation-specific operations
I WANT an explicitly declared compatibility boundary that localizes
exemptions by file boundary rather than scattered suppression comments
SO THAT the general rules stay strict while narrow portability paths remain possible.

## 0004 reviewer

AS a code reviewer
I WANT every effect, mutation, and control decision to appear as explicit
statement-level structure
SO THAT value computation and state transitions are distinguishable by inspection.
