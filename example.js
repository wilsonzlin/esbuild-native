const esbuild = require('.');

const opt = esbuild.createTransformOptions({
  sourceMap: 0,
  target: 0,
  engines: [],
  strictNullishCoalescing: false,
  strictClassFields: false,
  minifyWhitespace: true,
  minifyIdentifiers: true,
  minifySyntax: true,
  jsxFactory: '',
  jsxFragment: '',
  defines: [],
  pureFunctions: [],
  sourceFile: '',
  loader: 0,
});
const res = esbuild.transform(Buffer.from('let a = 1'), opt);
res.then(res => {
  console.log(res);
  console.log(res.js.toString());
  console.log(res.jsSourceMap.toString());
}, console.error);
