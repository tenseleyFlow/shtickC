Name:           shtick
Version:        1.0.0
Release:        1%{?dist}
Summary:        Shell configuration manager with multi-shell support

License:        MIT
URL:            https://github.com/tenseleyFlow/shtick
Source0:        %{name}-%{version}.tar.gz

BuildArch:      x86_64
BuildRequires:  gcc
BuildRequires:  make
Requires:       bash

%description
shtick is a C port of a shell configuration manager that provides persistent 
aliases, environment variables, and functions across multiple shell environments.
It supports 16 different shells including bash, zsh, fish, and modern shells 
like nu, elvish, and xonsh.

Features:
- Persistent aliases, environment variables, and functions
- Group-based configuration management
- Support for 16 different shells
- Interactive function editor
- Shell completion generation
- Backup and restore functionality

%prep
%autosetup

%build
# Build with standard optimization
make %{?_smp_mflags}

%install
# Install to buildroot
install -d %{buildroot}%{_bindir}
install -m 755 shtick %{buildroot}%{_bindir}/shtick

# Install documentation
install -d %{buildroot}%{_docdir}/%{name}
install -m 644 README.md %{buildroot}%{_docdir}/%{name}/

%files
%doc README.md
%{_bindir}/shtick
%{_docdir}/%{name}/

%changelog
* Sun Aug 24 2025 mfw <espadonne@outlook.com> - 1.0.0-1
- Initial RPM release
- C port of shell configuration manager
- Support for 16 different shells
- Group-based configuration management