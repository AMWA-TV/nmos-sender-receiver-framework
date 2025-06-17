check_env_var() {
    local name=$1
    if [[ -z "${!name+x}" ]]; then
        echo ">>> ERROR: ${name} must be defined"
        exit 1
    fi
}
