export const keyboardKey = {
    $tag: "keyboard-key",
    template: `
    <span class="keyboard-key">
        <slot/>
    </span>
    `,
}

export const keyCombo = {
    $tag: "key-combo",
    computed: {
        keys() {
            if (!this.$slots.default) {
                return [];
            }
            const keyString = this.$slots.default()[0].children;
            return keyString.split(" + ");
        }
    },
    template: `
    <span class="key-combo" title="Keyboard shortcut">

        <span class="icon">
        
            <i class="fas fa-keyboard fa-sm"/>
            
        </span>

        <span class="path">

            <keyboard-key v-for="(key, idx) in keys" :key="key">

                {{key}}

                <span v-if="idx < (keys.length - 1)" class="separator">

                    +

                </span>

            </keyboard-key>

        </span>
        
    </span>
    `,
};
