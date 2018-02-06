# Envoy fuzz testing

Envoy is fuzz tested via [OSS-Fuzz](https://github.com/google/oss-fuzz). We
follow the best practices described in the OSS-Fuzz [ideal integration
page](https://github.com/google/oss-fuzz/blob/master/docs/ideal_integration.md).

## Test environment

Tests should be unit test-like, fast and not require writable access to the filesystem (beyond
temporary files), network (including loopback) or multiple processes. See the
[ClusterFuzz
environment](https://github.com/google/oss-fuzz/blob/master/docs/fuzzer_environment.md)
for further details.

## Corpus

Every fuzz test must comes with a *corpus*. A corpus is a set of files that
provide example valid inputs. Fuzzing libraries will use this seed corpus to
drive mutations, e.g. via evolutionary fuzzing, to explore interesting parts of
the state space.

The corpus also acts as a quick regression test for evaluating the fuzz tests
without the help of a fuzzing library.

The corpus is located in a directory underneath the fuzz test. E.g. suppose you
have
[`test/common/common/base64_fuzz_test.cc`](../../test/common/common/base64_fuzz_test.cc),
a corpus directory
[`test/common/common/base64_corpus`](../../test/common/common/base64_corpus) should
exist, populated with files that will act as the corpus.

## Test driver

Your fuzz test will ultimately be driven by a simple interface:

```c++
DEFINE_FUZZER(const uint8_t* data, size_t size) {
  // Your test code goes here
}
```

It is up to your test `DEFINE_FUZZER` implementation to map this buffer of data to
meaningful semantics, e.g. a stream of network bytes or a protobuf binary input.

The fuzz test will be executed in two environments:

1. Under Envoy's fuzz test driver when run in the Envoy repository with
   `bazel test //test/path/to/some_fuzz_test`. This provides a litmus test
   indicating that the test passes CI and basic sanitizers on the supplied
   corpus.

2. Via fuzzing library test drivers in OSS-Fuzz. This is where the real fuzzing
   takes places on a VM cluster and the seed corpus is used by fuzzers to
   explore the state space.

## Defining a new fuzz test

1. Write a fuzz test module implementing the `DEFINE_FUZZER`
   interface. E.g.
   [`test/common/common/base64_fuzz_test.cc`](../../test/common/common/base64_fuzz_test.cc).

2. Define an `envoy_cc_fuzz_test` target, see `base64_fuzz_test` in
   [`test/common/common/BUILD`](../../test/common/common/BUILD).

3. Create the seed corpus directory and populate it with at least one example
   input. E.g.
   [`test/common/common/base64_corpus`](../../test/common/common/base64_corpus).

4. Run the `envoy_cc_fuzz_test` target. E.g. `bazel test
   //test/common/common:base64_fuzz_test`.

## Protobuf fuzz tests

We also have integration with
[libprotobuf-mutator](https://github.com/google/libprotobuf-mutator), allowing
tests built on a protobuf input to work directly with a typed protobuf object,
rather than a raw buffer. The interface to this is as described at
https://github.com/google/libprotobuf-mutator#integrating-with-libfuzzer:

```c++
DEFINE_PROTO_FUZZER(const MyMessageType& input) {
  // Your test code goes here
}
```

## Running fuzz tests locally

Within the Envoy repository, we have various `*_fuzz_test` targets. When run
under `bazel test`, these will exercise the corpus as inputs but not actually
link and run against any fuzzer (e.g.
[`libfuzzer`](https://llvm.org/docs/LibFuzzer.html)). The actual fuzzing is
performed by the [oss-fuzz](https://github.com/google/oss-fuzz) project, with
results provided on the [ClusterFuzz dashboard](https://oss-fuzz.com).

It is possible to run against fuzzers locally by using the `oss-fuzz` Docker
image. This is recommended when writing new fuzz tests to check if they pick up
any low hanging fruit (i.e. what you can find on your local machine vs. the fuzz
cluster).

1. `git clone https://github.com/google/oss-fuzz.git`
2. `cd oss-fuzz`
3. `python infra/helper.py build_image envoy`
4. `python infra/helper.py build_fuzzers --sanitizer=address envoy <path to
   envoy source tree>`. The path to the Envoy source tree can be omitted if you
   want to consume Envoy from GitHub at HEAD/master.
5. `python infra/helper.py run_fuzzer envoy <fuzz test target>`. The fuzz test
   target will be the test name, e.g. `server_fuzz_test`.

If there is a crash, `run_fuzzer` will emit a log line along the lines of:

```
artifact_prefix='./'; Test unit written to ./crash-db2ee19f50162f2079dc0c5ba24fd0e3dcb8b9bc
```

The test input can be found in `build/out/envoy`, e.g.
`build/out/envoy/crash-db2ee19f50162f2079dc0c5ba24fd0e3dcb8b9bc`. For protobuf
fuzz tests, this will be in text proto format.

To test and validate fixes to the crash, add it to the corpus in the Envoy
source directory for the test, e.g. for `server_fuzz_test` this is
`test/server/corpus`, and run `bazel test`, e.g. `bazel test
//test/server:server_fuzz_test`. These crash cases can be added to the corpus in
followup PRs to provide fuzzers some interesting starting points for invalid
inputs.