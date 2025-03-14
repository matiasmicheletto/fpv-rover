const padStart = (number, length) => { // Only works for positive numbers
    let str = '' + number; // Convert to string
    while (str.length < length) str = '0' + str;
    return str;
};

const clamp = (value, min, max) => {
    return Math.min(Math.max(value, min), max);
};

const toPercentage = (value, min, max) => {
    return (value - min) / (max - min) * 100;
};

const round = (value, decimals = 2) => {
    return Number(Math.round(value+'e'+decimals)+'e-'+decimals);
};