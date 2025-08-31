# Experiments

This folder contains experiments used to test techniques, benchmark performance, or as a sandbox for trying third-party libraries.

Example of goals from experiments:

1. Test the validity/performance of a new approach before integrating.
2. Isolate slow existing code to diagnose the root cause.
3. Build something complex (e.g., algorithm, class, etc.) without the burden of building the entire app.

## Maintaining

Please note that all experiments will be compiled along with the other GN targets. Because of this, it is best to ensure they are either **isolated** or are **minimally dependent** on application code. If an experiment is no longer needed or too difficult to maintain, it is best to remove it.
