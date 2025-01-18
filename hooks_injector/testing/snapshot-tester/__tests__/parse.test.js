const parseFile = require('../parse-file');
test('parse file', () => {
  const result = parseFile("/Users/rohanraja/src/codevines/ide1/first_party/xtrace-core/hooks_injector/cpp_hooks_injector/tests/inp.cc");
  expect(result).toMatchSnapshot();
});
