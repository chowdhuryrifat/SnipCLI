class Snipcli < Formula
  desc "High-performance algorithmic snippet manager CLI for competitive programmers"
  homepage "https://github.com/chowdhuryrifat/SnipCLI"
  url "https://github.com/chowdhuryrifat/SnipCLI/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "<SHA256_SOURCE_TARBALL>"
  license "MIT"
  head "https://github.com/chowdhuryrifat/SnipCLI.git", branch: "dev"

  depends_on "cmake" => :build

  def install
    system "cmake", "-S", ".", "-B", "build",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DSNIPCLI_BUILD_TESTS=OFF",
                    *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"

    # In addition to CMake install, guarantee Homebrew prefix completion paths
    bash_completion.install "completions/snip.bash" => "snip"
    zsh_completion.install "completions/_snip"
    fish_completion.install "completions/snip.fish"
    man1.install "man/snip.1"
  end

  test do
    assert_match "SnipCLI version #{version}", shell_output("#{bin}/snip --version")

    # Smoke test: add and retrieve snippet
    test_storage = testpath/"snippets.json"
    ENV["SNIPCLI_STORAGE_PATH"] = test_storage.to_s

    system "#{bin}/snip", "add", "--title", "TestSnippet", "--lang", "cpp", "--code", "int main(){return 0;}"
    assert_match "TestSnippet", shell_output("#{bin}/snip get --title TestSnippet")
  end
end
