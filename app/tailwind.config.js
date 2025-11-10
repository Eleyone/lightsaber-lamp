/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    "./src/**/*.{html,js}"
  ],
  darkMode: "class",
  theme: {
    extend: {},
    fontFamily: {
      sans: ["-apple-system", "BlinkMacSystemFont", "Segoe UI", "Roboto", "Helvetica Neue", "sans-serif"],
      body: ["-apple-system", "BlinkMacSystemFont", "Segoe UI", "Roboto", "Helvetica Neue", "sans-serif"],
      mono: ["ui-monospace", "monospace"],
    },
  },
}
