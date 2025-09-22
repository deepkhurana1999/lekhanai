## Contributing to the Backend (C++)

### Prerequisites & Dependencies
- Ensure you have a C++23 compatible compiler (e.g., GCC 13+, Clang 16+).
- Install CMake (version 3.28.3 or newer).
- Dependencies are managed via Docker and CMake. See `backend/docker/Dockerfile.dev` for required packages (cpprestsdk, nlohmann_json, websocket++, OpenBLAS, etc.).
- Submodules: `libdatachannel` and `whisper.cpp` are included as git submodules and initialized by Docker build scripts.

### Building the Backend
- **Recommended:** Use Docker for a consistent build environment:
	```sh
	docker-compose up --build
	```
- **Manual build:**
	```sh
	cd backend
	cmake -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build -j
	```
- The backend binary is built as `build/hikki-backend`.

### Code Style
- Follow modern C++ best practices (RAII, smart pointers, const-correctness).
- Use `snake_case` for variables/functions, `CamelCase` for types/classes.
- Document public APIs with comments.
- Keep code modular: separate logic into processors, servers, and config modules.

### Making Changes
1. Fork the repository and create a feature branch.
2. Make your changes with clear, atomic commits.
3. Ensure code builds and passes all tests (add tests if possible).
4. Run `clang-format` or similar tool to ensure code style consistency.
5. Open a Pull Request (PR) with a clear description of your changes.

### Pull Request Process
- All PRs are reviewed for code quality, clarity, and test coverage.
- Address review comments promptly.
- Squash commits if requested.
- Ensure your branch is up to date with `master` before merging.
