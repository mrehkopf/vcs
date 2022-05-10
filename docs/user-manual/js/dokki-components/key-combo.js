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
    <span class="key-combo">

        <i class="fas fa-keyboard fa-sm" style="margin-right: 0.3em"/>

        <keyboard-key v-for="key in keys" :key="key">

            {{key}}

        </keyboard-key>
        
    </span>
    `,
};
