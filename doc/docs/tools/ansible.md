<!-- markdownlint-disable-file MD013 MD033 -->
# Ansible Collection

The BlueChi project offers an [Ansible collection](https://galaxy.ansible.com/ui/repo/published/eclipse_bluechi/bluechi/docs/) for effortless provisioning of BlueChi on systems.

To utilize Ansible on a CentOS host and install the **eclipse_bluechi.bluechi** collection from Ansible Galaxy, use to the following steps.

## Install Ansible on CentOS

Ensure that Ansible is installed.

```bash
sudo dnf install ansible
```

## Install Eclipse BlueChi collection

```bash
ansible-galaxy collection install eclipse_bluechi.bluechi
```

## Verify Installation

You can verify that the collection has been installed by checking the installed collections:

```bash
ansible-galaxy collection list | grep -i bluechi
eclipse_bluechi.bluechi       1.0.0
```

## Create inventory.yml

Define FQDN for the controller and agents as showed below.

```bash
all:
  hosts:
  children:
    bluechi_controller:
      hosts:
        controller.example.com:
    bluechi_agents:
      hosts:
        agent-1.example.com:
        agent-2.example.com:
```

## Install bluechi in the machines listed in the inventory file

PLEASE NOTE: [Ansible assumes you are using SSH keys to connect to remote machines](https://docs.ansible.com/ansible/latest/inventory_guide/connection_details.html#setting-up-ssh-keys).

As example, to set up SSH agent to avoid retyping passwords, you can do:

```bash
ssh-agent bash
ssh-add ~/.ssh/id_rsa
```

Finally execute the ansible-playbook to run the installation process.

```bash
ansible-playbook -i inventory.yml eclipse_bluechi.bluechi.install
```
