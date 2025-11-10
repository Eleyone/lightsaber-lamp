import js from '@eslint/js';
import prettier from 'eslint-config-prettier';

export default [
    {
        ignores: ['node_modules', 'dist', 'build', '.parcel-cache', '*.min.js', 'src/html/**/*.html', '.eslintignore']
    },
    {
        files: ['src/**/*.js'],
        languageOptions: {
            ecmaVersion: 'latest',
            sourceType: 'module',
            globals: {
                window: 'readonly',
                document: 'readonly',
                navigator: 'readonly',
                console: 'readonly',
                Alpine: 'readonly',
                fetch: 'readonly',
                WebSocket: 'readonly',
                CustomEvent: 'readonly'
            }
        },
        rules: {
            ...js.configs.recommended.rules,
            'no-var': 'error',
            'prefer-const': 'error',
            'eqeqeq': ['error', 'always'],
            'curly': ['error', 'all'],
            'semi': ['error', 'always'],
            'quotes': ['error', 'single', { avoidEscape: true }],
            'indent': ['error', 4, { SwitchCase: 1 }],
            'comma-dangle': ['error', 'never'],
            'no-unused-vars': ['warn', { argsIgnorePattern: '^_' }],
            'no-console': ['warn', { allow: ['warn', 'error'] }],
            'arrow-spacing': 'error',
            'space-infix-ops': 'error',
            'keyword-spacing': 'error',
            'brace-style': ['error', '1tbs'],
            'object-curly-spacing': ['error', 'always'],
            'array-bracket-spacing': ['error', 'never']
        }
    },
    prettier
];
