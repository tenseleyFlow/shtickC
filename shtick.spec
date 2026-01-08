Name:           shtick
Version:        1.1.0
Release:        1%{?dist}
Summary:        Shell configuration manager with multi-shell support

License:        MIT
URL:            https://github.com/tenseleyFlow/shtickC
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

%global debug_package %{nil}

BuildRequires:  gcc
BuildRequires:  make
Requires:       bash

%description
shtick is a C port of a shell configuration manager that provides persistent 
aliases, environment variables, and functions across multiple shell environments.
It supports 16 different shells including bash, zsh, fish, and modern shells 
like nu, elvish, and xonsh.

Features:
- Auto-sourcing wrapper function (aliases/env/functions work immediately)
- Persistent aliases, environment variables, and functions
- Group-based configuration management
- Support for 16 different shells
- Interactive function editor
- Shell completion generation
- Backup and restore functionality

%prep
%autosetup -n shtickC-%{version}

%build
# Build with standard optimization
make %{?_smp_mflags}

%install
# Install to buildroot
install -d %{buildroot}%{_bindir}
install -m 755 shtick %{buildroot}%{_bindir}/shtick

# Install setup script
install -d %{buildroot}%{_datadir}/%{name}
install -m 755 setup.sh %{buildroot}%{_datadir}/%{name}/setup.sh

# Install documentation
install -d %{buildroot}%{_docdir}/%{name}
install -m 644 README.md %{buildroot}%{_docdir}/%{name}/
install -m 644 CLAUDE.md %{buildroot}%{_docdir}/%{name}/

%files
%doc README.md
%{_bindir}/shtick
%{_datadir}/%{name}/setup.sh
%{_docdir}/%{name}/

%post
cat << 'EOF'

================================================================================
shtick has been installed!
================================================================================

Quick Setup (Recommended):
---------------------------
Run the interactive setup script to enable auto-sourcing:

    /usr/share/shtick/setup.sh

This adds a wrapper function to your shell config so aliases/env/functions
work immediately without manual sourcing.

Alternative Setup:
------------------
Or add manually to your shell config (~/.bashrc, ~/.zshrc, etc.):

    source ~/.config/shtick/load_active.<shell>

Generate wrapper function:

    shtick wrapper bash    # for bash
    shtick wrapper zsh     # for zsh
    shtick wrapper fish    # for fish

Then copy the output to your shell config.

Getting Started:
----------------
    shtick init           # Show setup instructions
    shtick status         # Show current status
    shtick alias ll='ls -la'   # Create an alias

Documentation:
--------------
    /usr/share/doc/shtick/README.md

================================================================================
EOF

%postun
if [ $1 -eq 0 ]; then
cat << 'EOF'

================================================================================
shtick has been removed
================================================================================

If you used the auto-sourcing wrapper, remove it from your shell config:
    ~/.bashrc or ~/.zshrc or ~/.config/fish/config.fish

Your shtick configurations are still at:
    ~/.config/shtick/

To remove configurations:
    rm -rf ~/.config/shtick

================================================================================
EOF
fi

%changelog
* Wed Jan 08 2025 mfw <espadonne@outlook.com> - 1.1.0-1
- Add auto-sourcing wrapper function
- Add interactive setup script (setup.sh)
- Add post-install instructions
- Install CLAUDE.md documentation
- Fix compiler warning (use memcpy instead of strncpy)

* Sun Aug 24 2024 mfw <espadonne@outlook.com> - 1.0.0-1
- Initial RPM release
- C port of shell configuration manager
- Support for 16 different shells
- Group-based configuration management