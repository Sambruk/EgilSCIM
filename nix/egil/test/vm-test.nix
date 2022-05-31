{ dockerTools
, writeShellScript
, egil
, nixosTest,
}:

let
  inherit (dockerTools) pullImage;

  dockerImages = [
    (pullImage {
      imageName = "osixia/openldap";
      imageDigest = "sha256:d212a12aa728ccb4baf06fcc83dc77392d90018d13c9b40717cf455e09aeeef3";
      sha256 = "sha256-91CSC1kVGgMB7ZRoiuLjn5YpBZIgcZDcmJzwQNJYg/U=";
      os = "linux";
      arch = "amd64";
      finalImageTag = "1.2.4";
    })
  ];

  loadDockerImages = writeShellScript "load-docker-images.sh" ''
    for image in ${toString dockerImages}; do
      [ -f "$image" ] || continue
      docker load --input=$image
    done
  '';
in
nixosTest {
  name = "egil-test-suite";

  machine = {
    environment.systemPackages = [
      egil.test.suite
    ];

    virtualisation = {
      docker.enable = true;
      diskSize = 1024; # 512 is not enough
    };
  };

  testScript = ''
    machine.wait_for_unit("docker.service")
    machine.execute("${loadDockerImages}")
    machine.succeed("run_test_suite")
  '';
}
