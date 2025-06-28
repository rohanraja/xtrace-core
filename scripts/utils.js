async function runStep(step_name, fn, filterStr){
    if(filterStr){
        // Get all the steps to run
        const steps = filterStr.split(',');
        if(!steps.includes(step_name)){
            console.log(`### Skipping step: ${step_name}`);
            return;
        }
    }
    console.log(`### Starting step: ${step_name}`);
    try {
        await fn();
        console.log(`#### Completed step: ${step_name}`);
    } catch (error) {
        console.error(`### Error in step: ${step_name}`, error);
        throw new Error(`Step ${step_name} failed: ${error.message}`);
    }
}

module.exports = {
    runStep
}