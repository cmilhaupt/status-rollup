# CI/CD and Publishing

## Continuous Integration

The project uses GitHub Actions for continuous integration across multiple platforms and compilers:

- **CI workflow** (`.github/workflows/ci.yml`): Tests on Linux (GCC 10/11/13, Clang 14), macOS (Clang), and Windows (MSVC 2022) with Python 3.8-3.13
- **Wheels workflow** (`.github/workflows/wheels.yml`): Builds binary wheels for all platforms using cibuildwheel
- **Release workflow** (`.github/workflows/release.yml`): Automatically publishes to PyPI on version tags

## Publishing to PyPI

This project uses **Trusted Publishers** for secure, token-free PyPI publishing. To set up publishing for a fork:

1. **Configure PyPI Trusted Publisher**:
   - Go to https://pypi.org/manage/account/publishing/
   - Add a new trusted publisher with:
     - **PyPI Project Name**: `status-rollup` (or your fork's name)
     - **Owner**: Your GitHub username/org
     - **Repository name**: `status-rollup`
     - **Workflow name**: `release.yml`
     - **Environment name**: `pypi`

2. **Create a release**:
   ```bash
   # Update version in CMakeLists.txt (line 2)
   # Commit changes
   git tag v1.0.1
   git push origin v1.0.1
   ```

3. The release workflow will automatically:
   - Build wheels for all platforms
   - Publish to PyPI using OIDC authentication
   - Create a GitHub release with artifacts

No API tokens or secrets needed!
