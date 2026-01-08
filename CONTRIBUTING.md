# Contributing to DiskNuke

Thank you for your interest in DiskNuke!

## How can I contribute?

### Reporting Bugs
- Use GitHub Issues
- Describe the problem in detail
- Specify operating system and version
- Include error outputs

### Suggesting Features
- Open an issue with the "enhancement" label
- Describe the use case
- Explain why the feature would be useful

### Contributing Code

1. **Fork** the repository
2. **Clone** your fork
3. Create a **Feature Branch**: `git checkout -b feature/amazing-feature`
4. **Commit** your changes: `git commit -m 'Add amazing feature'`
5. **Push** to the branch: `git push origin feature/amazing-feature`
6. Open a **Pull Request**

### Code Style
- Follow the existing C code style
- Comment complex logic
- Add tests for new features

### Security
- Do NOT report security issues publicly
- Contact the maintainers directly
- Use responsible disclosure

## Setting Up Development Environment

```bash
git clone https://github.com/YOUR_USERNAME/disknuke.git
cd disknuke
mkdir build && cd build
cmake ..
make
```

## Running Tests

```bash
cd build
ctest --verbose
```

Thank you for your support! 🎉
