import math

def calculate_effort_and_time(sloc, project_type="semi-detached", actual_effort=None):
    """
    Calculate Effort (in person-months), Development Time (in months), and Team Size using the COCOMO model.

    Parameters:
        sloc (int): Source Lines of Code.
        project_type (str): Project type - 'organic', 'semi-detached', or 'embedded'.
        actual_effort (float, optional): If provided, reverse-engineers the KLOC to fit actual effort.

    Returns:
        dict: Results containing Effort, Development Time, KLOC, and other calculated metrics.
    """
    # Define COCOMO parameters
    parameters = {
        "organic": {"a": 1.8, "b": 1.05, "c": 2.5, "d": 0.38},
        "semi-detached": {"a": 3.0, "b": 1.12, "c": 2.5, "d": 0.35},
        "embedded": {"a": 3.6, "b": 1.20, "c": 2.5, "d": 0.32},
    }

    if project_type not in parameters:
        raise ValueError("Invalid project type. Choose 'organic', 'semi-detached', or 'embedded'.")

    params = parameters[project_type]
    a, b, c, d = params["a"], params["b"], params["c"], params["d"]

    # Convert SLOC to KLOC
    if actual_effort:
        kloc = (actual_effort / a) ** (1 / b)
        sloc = kloc * 1000
    else:
        kloc = sloc / 1000.0

    # Calculate Effort
    effort = a * (kloc ** b)

    # Calculate Development Time
    time = c * (effort ** d)

    # Results
    results = {
        "SLOC": sloc,
        "KLOC": kloc,
        "Effort (person-months)": effort,
        "Development Time (months)": time,
        "Team Size": effort / time if time > 0 else 1,
    }

    return results

if __name__ == "__main__":
    print("COCOMO Calculator")
    print("==================")
    sloc = input("Enter SLOC (Source Lines of Code) or press Enter to reverse-engineer: ").strip()
    project_type = input("Enter project type (organic, semi-detached, embedded) [default: semi-detached]: ").strip().lower() or "semi-detached"
    actual_effort = input("Enter actual effort in person-months (if available): ").strip()

    try:
        sloc = int(sloc) if sloc else None
        actual_effort = float(actual_effort) if actual_effort else None
        if not sloc and not actual_effort:
            raise ValueError("Either SLOC or actual effort must be provided.")

        results = calculate_effort_and_time(sloc, project_type, actual_effort)
        print("\nResults:")
        for key, value in results.items():
            print(f"{key}: {value:.2f}" if isinstance(value, float) else f"{key}: {value}")

    except Exception as e:
        print(f"Error: {e}")
